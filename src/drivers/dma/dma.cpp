#include "driver.hpp"
#include <stdio.h>

enum _DMA_STATE {
  ST_UNKNOWN   = 0,
  ST_IDLE      = 1,
  ST_BUSY      = 2,
  ST_FINISHED  = 3,
};
enum _BLOCK_STATE{
  UNKNOWN   = -1,
  BLOCK_EMPTY     = 0,
  BLOCK_BUSY   = 1,
  BLOCK_FULL      = 2
};

DMA::DMA (Nysa *nysa, Driver *driver, uint32_t dev_addr, bool debug){
  this->nysa           = nysa;
  this->driver         = driver;
  this->dev_addr       = dev_addr;
  this->debug          = debug;

  this->SIZE           = 0;
  this->BASE[0]        = 0;
  this->BASE[1]        = 0;
  this->REG_BASE[0]    = REG_UNINITIALIZED;
  this->REG_SIZE[0]    = REG_UNINITIALIZED;
  this->REG_BASE[1]    = REG_UNINITIALIZED;
  this->REG_SIZE[1]    = REG_UNINITIALIZED;
  this->strategy       = CADENCE;
  this->blocking       = true;
  this->timeout        = 1000;
  this->block_state[0] = UNKNOWN;
  this->block_state[1] = UNKNOWN;
  this->read_state     = ST_UNKNOWN;
  this->test_bit       = 0;
}
DMA::~DMA(){
}

/*
 *  Setup the DMA Controller
 *
 *  \param mem_base0: Block 0 base address in memory, this is where the DMA
 *    controller will read/write the first block of data
 *  \param mem_base1: Block 1 base address in memory, this is where the DMA
 *    controller will read/write the second block of data
 *  \param size: The size of the data to read/write in memory
 *  \param reg_status: address of the status register
 *  \param reg_base0: address of the block 0 base register
 *  \param reg_size0: address of the block 0 size register
 *  \param reg_base1: address of the block 1 base register
 *  \param reg_size1: address of the block 1 size register
 *  \param blocking:
 *    true: read/write will block until data is transfered
 *    false: read/write will return immediately if a transfer is finished or
 *      not
 *  \param strategy: RXTX_STRATEGY
 *    IMMEDIATE: read/write data as fast as possible
 *    CADENCE: pace the transfer (as a read/write buffer is finished the next
 *      buffer transaction begins)
 *    SINGLE_BUFFER: only perform one read/write at a time, do not try and
 *      use a dual buffer
 *
 *  \retval  0: all fine
 *
 *  \remark This should be called before all functions
*/

int DMA::setup( uint32_t      mem_base0,
                uint32_t      mem_base1,
                uint32_t      size,
                uint32_t      reg_status,
                uint32_t      reg_base0,
                uint32_t      reg_size0,
                uint32_t      reg_base1,
                uint32_t      reg_size1,
                bool          blocking,
                RXTX_STRATEGY strategy){

  uint32_t status       = 0;
  this->REG_STATUS      = reg_status;
  this->SIZE            = size;
  this->BASE[0]         = mem_base0;
  this->BASE[1]         = mem_base1;
  this->REG_BASE[0]     = reg_base0;
  this->REG_SIZE[0]     = reg_size0;
  this->REG_BASE[1]     = reg_base1;
  this->REG_SIZE[1]     = reg_size1;
  this->blocking        = blocking;
  this->strategy        = strategy;

  //Setup the core
  this->nysa->write_register(this->dev_addr, REG_BASE[0], this->BASE[0]);
  this->nysa->write_register(this->dev_addr, REG_BASE[1], this->BASE[1]);
  return 0;
};

/*
 *  Setup the DMA Controller for write operations
 *
 *  \param mem_base0: Block 0 base address in memory, this is where the DMA
 *    controller will read/write the first block of data
 *  \param mem_base1: Block 1 base address in memory, this is where the DMA
 *    controller will read/write the second block of data
 *  \param size: The size of the data to read/write in memory
 *  \param reg_status: address of the status register
 *  \param reg_base0: address of the block 0 base register
 *  \param reg_size0: address of the block 0 size register
 *  \param reg_base1: address of the block 1 base register
 *  \param reg_size1: address of the block 1 size register
 *  \param blocking:
 *    true: read/write will block until data is transfered
 *    false: read/write will return immediately if a transfer is finished or
 *      not
 *  \param strategy: RXTX_STRATEGY
 *    IMMEDIATE: read/write data as fast as possible
 *    CADENCE: pace the transfer (as a read/write buffer is finished the next
 *      buffer transaction begins)
 *    SINGLE_BUFFER: only perform one read/write at a time, do not try and
 *      use a dual buffer
 *
 *  \retval  0: all fine
 *
 *  \remark This should be called before all functions
*/


int DMA::setup_write( uint32_t      mem_base0,
                      uint32_t      mem_base1,
                      uint32_t      size,
                      uint32_t      reg_status,
                      uint32_t      reg_base0,
                      uint32_t      reg_size0,
                      uint32_t      reg_base1,
                      uint32_t      reg_size1,
                      bool          blocking,
                      RXTX_STRATEGY strategy){
  this->writing = true;
  return this->setup( mem_base0,
                      mem_base1,
                      size,
                      reg_status,
                      reg_base0,
                      reg_size0,
                      reg_base1,
                      reg_size1,
                      blocking,
                      strategy);
}

/*
 *  Setup the DMA Controller for read operations
 *
 *  \param mem_base0: Block 0 base address in memory, this is where the DMA
 *    controller will read/write the first block of data
 *  \param mem_base1: Block 1 base address in memory, this is where the DMA
 *    controller will read/write the second block of data
 *  \param size: The size of the data to read/write in memory
 *  \param reg_status: address of the status register
 *  \param reg_base0: address of the block 0 base register
 *  \param reg_size0: address of the block 0 size register
 *  \param reg_base1: address of the block 1 base register
 *  \param reg_size1: address of the block 1 size register
 *  \param blocking:
 *    true: read/write will block until data is transfered
 *    false: read/write will return immediately if a transfer is finished or
 *      not
 *  \param strategy: RXTX_STRATEGY
 *    IMMEDIATE: read/write data as fast as possible
 *    CADENCE: pace the transfer (as a read/write buffer is finished the next
 *      buffer transaction begins)
 *    SINGLE_BUFFER: only perform one read/write at a time, do not try and
 *      use a dual buffer
 *
 *  \retval  0: all fine
 *
 *  \remark This should be called before all functions
*/
int DMA::setup_read(  uint32_t      mem_base0,
                      uint32_t      mem_base1,
                      uint32_t      size,
                      uint32_t      reg_status,
                      uint32_t      reg_base0,
                      uint32_t      reg_size0,
                      uint32_t      reg_base1,
                      uint32_t      reg_size1,
                      bool          blocking,
                      RXTX_STRATEGY strategy){
  this->writing = false;
  return this->setup( mem_base0,
                      mem_base1,
                      size,
                      reg_status,
                      reg_base0,
                      reg_size0,
                      reg_base1,
                      reg_size1,
                      blocking,
                      strategy);
}



/*
 *  Setup the DMA transfer strategy:
 *    The strategy dictates the behavior of the transfers which will vary
 *    for different applications
 *
 *    Examples for each type:
 *      IMMEDIATE: High throughput devices like a hard drive are not time
 *        dependent
 *      CADENCE: Pace of the incomming data is more important. This is good
 *        for data that is time dependant such as image data from a camera
 *        or microphone
 *      SINGLE_BUFFER: The incomming data speed is not important but the
 *        device that the device interface with has a relatively large amount
 *        of data that it outputs
 *
 *  \param strategy: RXTX_STRATEGY
 *    IMMEDIATE: read/write data as fast as possible
 *    CADENCE: pace the transfer (as a read/write buffer is finished the next
 *      buffer transaction begins)
 *    SINGLE_BUFFER: only perform one read/write at a time, do not try and
 *      use a dual buffer
 *
*/
void DMA::set_strategy (RXTX_STRATEGY strategy){
  this->strategy = strategy;
};

/*
 *  Setup the status transfer bits
 *    Core designers have a lot of flexibility to define the behavior of each
 *    register including the status register. This function allows the driver
 *    writer to set the appropriate bits in the status register
 *
 *    Example:
 *      finished0:  0 ->  0x00000001
 *      finished1:  1 ->  0x00000002
 *      empty0:     2 ->  0x00000004
 *      empty1:     3 ->  0x00000008
 *
 *  \param finished0: bit address of 'finished' flag for block 0
 *  \param finished1: bit address of 'finished' flag for block 1
 *  \param empty0: bit address of 'empty' flag for block 0
 *  \param empty1: bit address of 'empty' flag for block 1
 *
*/
void DMA::set_status_bits(  uint8_t finished0,
                            uint8_t finished1,
                            uint8_t empty0,
                            uint8_t empty1){

    this->status_bit_finished[0] = 1 << finished0;
    this->status_bit_finished[1] = 1 << finished1;
    this->status_bit_empty[0]    = 1 << empty0;
    this->status_bit_empty[1]    = 1 << empty1;
}

/*
 *  Enable blocking reads
 *    select whether a read/write transaction will block when data is not
 *    available
 *
 *  \param enable: Enable blocking mode (Default true)
 *
*/
void DMA::enable_blocking(bool enable){
  this->blocking = enable;
}

/*
 *  Write data using the DMA
 *  The behavior of this function is set by the strategy function
 *  'set_strategy' or in 'setup'
 *    blocking: write will block until all data is written
 *    strategy: the DMA transfer will behave differently. based on the
 *      enumerated value set either in 'setup' or in 'set_strategy'
 *
 *  \param buffer: a pointer to the buffer of data to send
 *
*/
int DMA::write(uint8_t *buffer){
  int retval = 0;
  bool finished = false;
  uint32_t status;
  uint32_t interrupts;

  while (!finished){
//    printf ("%s(): Main loop\n", __func__);
    //Get the current status
    status = this->driver->read_register(this->REG_STATUS);
//    printf ("%s(): Status Register: 0x%08X\n", __func__, status);
    //Check if anything is ready
    if ((status & (this->status_bit_empty[0] | this->status_bit_empty[1])) == 0){ 
      //neither block is ready
      //wait for interrupts
      while (this->blocking){
//        //printf ("%s(): Blocking...\n", __func__);
        //If the user is okay with waiting just keep waiting for interrupts
        this->nysa->wait_for_interrupts(1000, &interrupts);
        if (this->driver->is_interrupt_for_device(interrupts)) {
//          printf ("%s(): Found an interrupt\n", __func__);
          break;
        }
        else {
//          printf ("%s(): Didn't find interrupts\n", __func__);
          status = this->driver->read_register(this->REG_STATUS);
        }
      }
      status = this->driver->read_register(this->REG_STATUS);
      if (status & (this->status_bit_empty[0] || this->status_bit_empty[1])){
        //empty status not found
        printd ("Second status found no empty block available (this could be caused by a non-block return\n");
        retval = 1;
        finished = true;
        break;
      }
    }
    this->process_status(status);

    //Test whether we can send data as fast as possible or whether we need
    //both blocks empty efore we can send more data
    switch (this->strategy){
      case (IMMEDIATE):
      case (CADENCE):
//        printf ("%s(): In Strategy Switch...\n", __func__);
//        printf ("%s(): block state [0]: 0x%08X\n", __func__, this->block_state[0]);
//        printf ("%s(): block state [1]: 0x%08X\n", __func__, this->block_state[1]);
        if ((this->block_state[0] == BLOCK_EMPTY) ||
            (this->block_state[1] == BLOCK_EMPTY)){
          //A FIFO is available, start sending data down NOW
          if (this->block_state[0] == BLOCK_EMPTY){
//            printf ("%s(): Writing to register 0\n", __func__);
            this->nysa->write_memory(this->BASE[0], &buffer[0], this->SIZE);
            this->driver->write_register(this->REG_SIZE[0], this->SIZE);
          }
          else {
//            printf ("%s(): Writing to register 1\n", __func__);
            this->nysa->write_memory(this->BASE[1], &buffer[1], this->SIZE);
            this->driver->write_register(this->REG_SIZE[1], this->SIZE);
          }
          finished = true;
          retval = 0;
        }
        break;
      case (SINGLE_BUFFER):
        if ((this->block_state[0] == BLOCK_EMPTY) &&
            (this->block_state[1] == BLOCK_EMPTY)){

          this->nysa->write_memory(this->BASE[0], &buffer[0], this->SIZE);
          this->driver->write_register(this->REG_SIZE[0], this->SIZE);
        }
        break;
      default:
        retval = -2;  //unknown strategy
        break;
    }//switch (strategy)
  } //while (!finished)
  return retval;
}

/*
 *  Process the incomming status
 *
 *  \param buffer: incomming status from the core
 *
*/
#define DEBUG_PROCESS false

void DMA::process_status(uint32_t status){
  if (this->writing){
//    //printf ("%s(): status_bit_empty[0] == 0x%08X\n", __func__, this->status_bit_empty[0]);
//    //printf ("%s(): status_bit_empty[1] == 0x%08X\n", __func__, this->status_bit_empty[1]);
    if (status & this->status_bit_empty[0]){
      this->block_state[0] = BLOCK_EMPTY;
    }
    else{
      this->block_state[0] = BLOCK_BUSY;
    }
    if (status & this->status_bit_empty[1]){
      this->block_state[1] = BLOCK_EMPTY;
    }
    else{
      this->block_state[1] = BLOCK_BUSY;
    }
  }
  else { //reading
//    //printf ("%s(): Processing Read Status: 0x%08X\n", __func__, status);
    if (status & this->status_bit_finished[0]){
//      if (DEBUG_PROCESS) printf ("%s(): Block 0: Full\n", __func__);
      this->block_state[0] = BLOCK_FULL;
    }
    else if (status & this->status_bit_empty[0]){
//      if (DEBUG_PROCESS) printf ("%s(): Block 0: Empty\n", __func__);
      this->block_state[0] = BLOCK_EMPTY;
    }
    else{
//      if (DEBUG_PROCESS) printf ("%s(): Block 0: Working\n", __func__);
      this->block_state[0] = BLOCK_BUSY;
    }

    if (status & this->status_bit_finished[1]){
//      if (DEBUG_PROCESS) printf ("%s(): Block 1: Full\n", __func__);
      this->block_state[1] = BLOCK_FULL;
    }
    else if (status & this->status_bit_empty[1]){
//      if (DEBUG_PROCESS) printf ("%s(): Block 1: Empty\n", __func__);
      this->block_state[1] = BLOCK_EMPTY;
    }
    else{
//      if (DEBUG_PROCESS) printf ("%s(): Block 1: Working\n", __func__);
      this->block_state[1] = BLOCK_BUSY;
    }
  }
}

/*
 *  Read a block of data from the Memory
 *    Populate the buffer pointed to by uint8_t buffer passed in the size is
 *    always the size that is specified in 'SIZE' in 'setup'
 *
 *    The function will also manage the 'strategy' specified within 'setup' or
 *    'set_strategy' This is important because the core will launch interrupts
 *    indicating the state of the read status which is processed by successive
 *    calls to the read function.
 *
 *  \param buffer: pointer to a buffer of unsigned bytes that will be populated
 *    by the read result
 *
 *  \retval  0: all fine
 *           1: non-blocking time out
 *
*/

int DMA::read(uint8_t *buffer){
  int retval;
  uint32_t status;
  uint32_t interrupts;
  uint32_t read_size;
  uint32_t pos = 0;
  bool block = this->blocking;
  bool finished = false;
//  //printf ("%s(): Entered\n", __func__);

  //get the current status
  while (!finished){
    switch (this->read_state){
      case(ST_IDLE):
//        printf ("%s(): IDLE State, requesting data\n", __func__);
        //No transactions have started
        this->block_select = 0;
        this->driver->write_register(this->REG_SIZE[0], this->SIZE);
        if (this->strategy == IMMEDIATE){
          this->driver->write_register(this->REG_SIZE[1], this->SIZE);
        }
        this->read_state = ST_BUSY;
        break;
      case(ST_BUSY):
        //retval = this->nysa->wait_for_interrupts(2, &interrupts);
//        printf ("%s(): BUSY State\n", __func__);
        status = this->driver->read_register(this->REG_STATUS);
//        printf ("\tStatus Register: 0x%08X\n", status);
        this->process_status(status);
        if ((this->block_state[0] == BLOCK_EMPTY) &&
            (this->block_state[1] == BLOCK_EMPTY)){
          this->read_state = ST_UNKNOWN;
          break;
        }
        if ((this->block_state[0] != BLOCK_FULL) &&
            (this->block_state[1] != BLOCK_FULL)){
          do {
            retval = this->nysa->wait_for_interrupts(1000, &interrupts);
//            printf ("\tInterrupt Return: %d\n", retval);
            if (retval == 0){
              if (this->driver->is_interrupt_for_device(interrupts)){
                  //we got some data
//                  printf ("\tInterrupt for us!\n");
                  //break;
              }
            }
            status = this->driver->read_register(this->REG_STATUS);
//            printf ("\tStatus Register: 0x%08X\n", status);
            this->process_status(status);
            if ((this->block_state[0] == BLOCK_FULL) ||
                (this->block_state[1] == BLOCK_FULL)){
              break;
            }
          } while (block);
        }
//        //printf ("\tBlock states: 0x%02X 0x%02X\n", this->block_state[0], this->block_state[1]);
        if ((this->block_state[0] == BLOCK_FULL) ||
            (this->block_state[1] == BLOCK_FULL) ){
          /*
          //there is some data to read
          if ((this->strategy == IMMEDIATE) || (this->strategy == CADENCE)){
            if (this->block_state[0] == BLOCK_EMPTY){
//              printf ("\tStart a read of block 0\n");
              this->driver->write_register(this->REG_SIZE[0], this->SIZE);
              this->block_state[0] = BLOCK_BUSY;
            }
            if (this->block_state[1] == BLOCK_EMPTY){
//              printf ("\tStart a read of block 1\n");
              this->driver->write_register(this->REG_SIZE[1], this->SIZE);
              this->block_state[1] = BLOCK_BUSY;
            }
          }
          */
          this->read_state = ST_FINISHED;
          continue;
        }
        retval = 1; //Timed out
        finished = true;
        break;
      case(ST_FINISHED):

        //FPGA has some data to process
//        printf ("%s(): ST_FINISHED, Reading data\n", __func__);
        if ((this->block_state[0] != BLOCK_FULL) &&
            (this->block_state[1] != BLOCK_FULL) ){
//          printf ("\tBoth blocks are not full, get the state\n");
          this->read_state = ST_UNKNOWN;
          continue;
        }

        //BOTH BLOCKS ARE READY
        if ((this->block_state[0] == BLOCK_FULL) &&
            (this->block_state[1] == BLOCK_FULL)){

//          printf ("\tBOTH BLOCKS ARE FULL\n");
          //both are finished, use the local variable as a tie breaker
          if (this->block_select == 0){
//            printf ("\t\t\t\t0\n");
            this->nysa->read_memory(this->BASE[0], buffer, this->SIZE);
            this->block_select = 1;
//            if (this->test_bit) printf ("\t\t\t\t\t\t\t\t\t\tFAIL!: Test bit should be 0\n");
            this->test_bit = 1;
            //status = this->driver->read_register(this->REG_STATUS);
            //if (this->strategy == IMMEDIATE) {
              this->driver->write_register(this->REG_SIZE[0], this->SIZE);
              this->block_state[0] = BLOCK_BUSY;
            //}
            //else {
            //  this->block_state[0] = BLOCK_EMPTY;
            //}
          }
          else {
//            printf ("\t\t\t\t1\n");
            this->nysa->read_memory(this->BASE[1], buffer, this->SIZE);
            this->block_select = 0;
//            if (!this->test_bit) printf ("\t\t\t\t\t\t\t\t\t\tFAIL!: Test bit should be 1\n");
            this->test_bit = 0;
            //status = this->driver->read_register(this->REG_STATUS);
            //if (this->strategy == IMMEDIATE) {
              this->driver->write_register(this->REG_SIZE[0], this->SIZE);
              this->block_state[1] = BLOCK_BUSY;
            //}
            //else {
            //  this->block_state[1] = BLOCK_EMPTY;
            //}
          }
          this->read_state = ST_FINISHED;
        }

        //SINGLE BLOCK READY
        else if (this->block_state[0] == BLOCK_FULL) {
//          printf ("\t\tBLOCK 0 IS FULL ONLY\n");
//          printf ("\t\t\t\t0\n");
          this->block_select = 1;
          this->nysa->read_memory(this->BASE[0], buffer, this->SIZE);
//          if (this->test_bit) printf ("\t\t\t\t\t\t\t\t\t\tFAIL!: Test bit should be 0\n");
          this->test_bit = 1;
          this->block_state[0] = BLOCK_EMPTY;
          //this->read_state = ST_IDLE;
          //if ((this->strategy == IMMEDIATE) || (this->strategy == CADENCE)){
            if (this->block_state[1] == BLOCK_EMPTY){
//              printf ("\t\t\tStart reading from block 1\n");
              this->driver->write_register(this->REG_SIZE[1], this->SIZE);
              this->block_state[1] == BLOCK_BUSY;
            }
          //}
          //if (this->strategy == IMMEDIATE){
          //  this->driver->write_register(this->REG_SIZE[0], this->SIZE);
          //  this->block_state[0] = BLOCK_BUSY;
          //  this->read_state = ST_BUSY;
          //}
          this->read_state = ST_BUSY;
        }
        else if (this->block_state[1] == BLOCK_FULL) {
//          printf ("\t\tBLOCK 1 IS FULL ONLY\n");
//          printf ("\t\t\t\t1\n");
          this->block_select = 0;
          this->nysa->read_memory(this->BASE[1], buffer, this->SIZE);
//          if (!this->test_bit) printf ("\t\t\t\t\t\t\t\t\t\tFAIL!: Test bit should be 1\n");
          this->test_bit = 0;
          this->block_state[1] = BLOCK_EMPTY;
          this->read_state = ST_IDLE;
          //if ((this->strategy == IMMEDIATE) || (this->strategy == CADENCE)){
            if (this->block_state[0] == BLOCK_EMPTY){
//              printf ("\t\t\tStart reading from block 0\n");
              this->driver->write_register(this->REG_SIZE[0], this->SIZE);
              this->block_state[0] == BLOCK_BUSY;
            }
          //  this->read_state = ST_BUSY;
          //}
          //if (this->strategy == IMMEDIATE){
            this->driver->write_register(this->REG_SIZE[1], this->SIZE);
            this->block_state[1] = BLOCK_BUSY;
          //  this->read_state = ST_BUSY;
          //}
          this->read_state = ST_BUSY;
        }
        retval = 0; //fine
        finished = true;
        break;
      case(ST_UNKNOWN):
      default:
        //We need to get the status
//        printf ("%s(): UNKNOWN Status\n", __func__);
        status = this->driver->read_register(this->REG_STATUS);
        this->process_status(status);
//        //printf ("\tBlock states: 0x%02X 0x%02X\n", this->block_state[0], this->block_state[1]);
        if ((this->block_state[0] == BLOCK_FULL) ||
            (this->block_state[1] == BLOCK_FULL) ){
          //there is some data to read
          this->read_state = ST_FINISHED;
//          //printf ("\tread state: 0x%02X\n", this->read_state);
        }
        else if ((this->block_state[0] == BLOCK_BUSY) ||
                 (this->block_state[1] == BLOCK_BUSY)){
          //The FPGA is busy working on something
          this->read_state = ST_BUSY;
//          //printf ("\tread state: 0x%02X\n", this->read_state);
        }

        else if ((this->block_state[0] == BLOCK_EMPTY) ||
                 (this->block_state[1] == BLOCK_EMPTY)){
          //The FPGA is not doing anything right now
//          printf ("\tThere is an empty block, go to IDLE!\n");
          this->read_state = ST_IDLE;
//          //printf ("\tread state: 0x%02X\n", this->read_state);
        }
        break;
    }//switch (read_state)
  }//while (!finished)
  return retval;
}

