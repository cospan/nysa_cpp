#include "driver.hpp"

#define EMPTY_MASK (this->status_bit_empty[0] & this->status_bit_empty[1])
#define READ_READY_MASK(x) (this->status_bit_empty[x] & this->status_bit_finished[x])


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
  this->read_state     = IDLE;
}
DMA::~DMA(){
}

/*
 *  Setup the DMA Controller
 *
 *  \param base0: Block 0 base address in memory, this is where the DMA
 *    controller will read/write the first block of data
 *  \param base1: Block 1 base address in memory, this is where the DMA
 *    controller will read/write the second block of data
 *  \param size: The size of the data to read/write in memory
 *  \param REG_STATUS: address of the status register
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

int DMA::setup( uint32_t REG_STATUS,
                uint32_t base0,
                uint32_t base1,
                uint32_t size,
                uint32_t reg_base0,
                uint32_t reg_size0,
                uint32_t reg_base1,
                uint32_t reg_size1,
                bool     blocking,
                RXTX_STRATEGY strategy){

  uint32_t status       = 0;
  this->REG_STATUS      = REG_STATUS;
  this->SIZE            = size;
  this->BASE[0]         = base0;
  this->BASE[1]         = base1;
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
 *  The behavior of this function is set by various flags by the user:
 *    blocking: write will block until all data is written
 *    strategy: the DMA transfer will behave differently. based on the
 *      enumerated value set either in 'setup' or in 'set_strategy'
 *
 *  \param buffer: a pointer to the buffer of data to send
 *  \param size: size: size of the buffer to write
 *
*/
int DMA::write(uint8_t *buffer, uint32_t size){
  uint32_t status;
  uint32_t interrupts;
  uint32_t write_size;
  uint32_t pos = 0;

  while (pos < size){
    //Get the current status
    status = this->driver->read_register(this->REG_STATUS);
    //Check if anything is ready
    if (status & (this->status_bit_empty[0] || this->status_bit_empty[1])){
      //neither block is ready
      //wait for interrupts
      while (this->blocking){
        //If the user is okay with waiting just keep waiting for interrupts
        this->nysa->wait_for_interrupts(1000, &interrupts);
        if (this->driver->is_interrupt_for_device(interrupts)) {
          break;
        }
      }
      status = this->driver->read_register(this->REG_STATUS);
      if (status & (this->status_bit_empty[0] || this->status_bit_empty[1])){
        return pos;
      }
    }
    //Test whether we can send data as fast as possible or whether we need
    //both blocks empty efore we can send more data
    if (this->strategy == IMMEDIATE &
        (status & EMPTY_MASK) != EMPTY_MASK){
      //Go back to looking through status
      //XXX: This could loop forever because the status above only checks if anything is empty not both
      continue;
    }
    //check if a block is available
    if (status & this->status_bit_empty[0]) {
      write_size = (size - pos);
      //Block 1 is ready
      //XXX: This needs to acpos for sizes that are not on the block boundary
      this->nysa->write_memory(this->BASE[0], &buffer[pos], write_size);
      this->driver->write_register(this->REG_SIZE[0], write_size);
      pos += write_size;
    }
    else {
      write_size = (size - pos);
      //Block 2 is ready
      //XXX: This needs to acpos for sizes that are not on the block boundary
      this->nysa->write_memory(this->BASE[1], &buffer[pos], write_size);
      this->driver->write_register(this->REG_SIZE[1], write_size);
      pos += write_size;
    }
  }
  return pos;
}
//Read
int DMA::read(uint8_t *buffer, uint32_t size){
  uint32_t status;
  uint32_t interrupts;
  uint32_t read_size;
  uint32_t pos = 0;

  while (pos < size){
    //get the current status
    status = this->driver->read_register(this->REG_STATUS);
    //Chck if anything is ready
    if (status & (READ_READY_MASK(0) | READ_READY_MASK(1))){
      //thing are not ready
      //wait for interrupts
      while (this->blocking){
        //If the user is okay with waiting just keep waiting for interrupts
        this->nysa->wait_for_interrupts(1000, &interrupts);
        if (this->driver->is_interrupt_for_device(interrupts)) {
          break;
        }
      }
      status = this->driver->read_register(this->REG_STATUS);
      if (status & (READ_READY_MASK(0) | READ_READY_MASK(1))){
        return pos;
      }
    }
    //There is space for the core to write data to memory
    if (status & (this->status_bit_empty[0] | this->status_bit_empty[1])) {
    }
  }
  return pos;
}

