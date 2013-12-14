#include "dionysus_local.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>
#include <malloc.h>

static double TimevalDiff(const struct timeval *a, const struct timeval *b){
    return (a->tv_sec - b->tv_sec) + 1e-6 * (a->tv_usec - b->tv_usec);
}

static int check_response(state_t *state, response_header_t * response){
  int retval = 0;
  if (state->debug){
    printf ("Response\n");
    printf ("\tID: %02X\n", response->id);
    printf ("\tCommand Status: %02X\n", response->status);
  }
  if (state->response_header.id != ID_RESPONSE){
    //Fail, ID does not match
    if (state->debug){
      printf ("ID Response != 0x%0X: %02X\n", ID_RESPONSE, state->response_header.id);
    }
    retval = -1;
  }
  //XXX: how to check the command response
  else if (state->response_header.status != ((~state->command_header.command) & 0xFF)){
    if (state->debug){
      printf ("Status Read != ~Command\n");
    }
    retval = -2;
  }
  if (state->command_header.command == PING){
    return retval;
  }
  if (state->command_header.command == WRITE){
    return retval;
  }
  state->read_data_count =  state->response_header.data_count[0] << 16;
  state->read_data_count |= state->response_header.data_count[1] << 8;
  state->read_data_count |= state->response_header.data_count[2];
  if (state->debug){
    printf ("Data Count (32-bit Data words): 0x%08X\n", state->read_data_count);
  }

  if (state->mem_response){
    state->read_mem_addr =  state->response_header.address.mem_addr[0] << 24;
    state->read_mem_addr |= state->response_header.address.mem_addr[1] << 16;
    state->read_mem_addr |= state->response_header.address.mem_addr[2] << 8;
    state->read_mem_addr |= state->response_header.address.mem_addr[3];
    if (state->debug){
      printf ("Memory Access @ 0x%08X\n", state->read_mem_addr);
    }
    retval = -3;
  }
  else {
    state->read_dev_addr = state->response_header.address.dev_addr;
    state->read_reg_addr = state->response_header.address.reg_addr[0] << 16;
    state->read_reg_addr |= state->response_header.address.reg_addr[1] << 8;
    state->read_reg_addr |= state->response_header.address.reg_addr[2];
    if (state->debug){
      printf ("Peripheral Access: Device: 0x%02X Register: 0x%06X\n", state->read_dev_addr, state->read_reg_addr);
    }
    retval = -4;
  }
  return retval;
}

static void print_transfer_status(struct libusb_transfer *transfer){
  printf ("Status: 0x%02X\n", transfer->status);
  switch(transfer->status){
    case(LIBUSB_TRANSFER_COMPLETED):
      printf ("Transfer Completed\n");
      break;
    case(LIBUSB_TRANSFER_ERROR):
      printf ("Transfer Error\n");
      break;
    case(LIBUSB_TRANSFER_TIMED_OUT):
      printf ("Transfer Timed Out\n");
      break;
    case(LIBUSB_TRANSFER_CANCELLED):
      printf ("Transfer Cancelled\n");
      break;
    case(LIBUSB_TRANSFER_STALL):
      printf ("Transfer Stalled\n");
      break;
    case(LIBUSB_TRANSFER_NO_DEVICE):
      printf ("Transfer No Device\n");
      break;
    case(LIBUSB_TRANSFER_OVERFLOW):
      printf ("Transfer Overflow\n");
      break;
    default:
      break;
  }
}

void Dionysus::usb_constructor(){
  //Initialize the read structure
  this->state                  = new state_t;

  this->state->transfer_index  = 0;
  this->state->usb_ctx         = NULL;
  this->state->usb_dev         = NULL;
  this->state->in_ep           = 0;
  this->state->out_ep          = 0;
  this->state->finished        = true;
  this->state->error           = 0;

  //User Data
  this->state->buffer          = NULL;
  this->state->buffer_pos      = 0;
  this->state->buffer_size     = 0;

  this->state->header_pos      = 0;
  this->state->header_size     = 0;

  //USB Transfer position
  this->state->usb_total_size  = 0;
  this->state->usb_size_left   = 0;
  this->state->usb_pos         = 0;
  this->state->usb_actual_pos  = 0;
  this->state->timeout         = 1000;

  //Transfer and Buffer Queue
  this->state->transfer_queue  = &this->transfer_queue;
  this->state->buffer_queue    = &this->buffer_queue;

  this->state->d               = this;
  this->state->debug           = debug;
  this->state->read_data_count = 0;
  this->state->read_dev_addr   = 0;
  this->state->read_reg_addr   = 0;
  this->state->read_mem_addr   = 0;
  this->state->mem_response    = false;

  for (int i = 0; i < NUM_TRANSFERS; i++){
    //uint8_t * buffer = new uint8_t[BUFFER_SIZE + 2]; //Add two for the modem status
    uint8_t * buffer = new uint8_t[BUFFER_SIZE]; //Add two for the modem status
    this->buffer_queue.push(buffer);
    this->buffers.push(buffer);
    struct libusb_transfer *transfer = NULL;
    transfer = libusb_alloc_transfer(0);
    this->transfer_queue.push(transfer);
    this->transfers.push(transfer);

    if (!transfer){
      printf ("Dionysus: Error while creating index %d usb transfer\n", i);
    }
  }
}

int Dionysus::usb_open(int vendor, int product){
  int retval = 0;
  //Setup the USB Device
  retval = ftdi_set_interface(this->ftdi, INTERFACE_A);
    CHECK_ERROR("Failed to set interface");
  if (this->debug) printf ("Dionysus: Open a context\n");
  retval = ftdi_usb_open(this->ftdi, vendor, product);
    CHECK_ERROR("Failed to open FTDI");
  this->reset();
  this->usb_is_open = true;
  retval = Dionysus::set_comm_mode();
  this->state->usb_ctx           = this->ftdi->usb_ctx;
  this->state->usb_dev           = this->ftdi->usb_dev;
  this->state->in_ep             = this->ftdi->in_ep;
  this->state->out_ep            = this->ftdi->out_ep;
  return retval;
}

void Dionysus::usb_destructor(){
  struct libusb_transfer *transfer = NULL;
  uint8_t * buffer = NULL;
  //Empty the working queues
  while (!this->transfer_queue.empty()){
    this->transfer_queue.pop();
  }
  //Empty the working queue
  while (!buffer_queue.empty()){
    this->buffer_queue.pop();
  }
  //Remove all the transfer_queue
  while (!this->transfers.empty()){
    transfer = this->transfers.front();
    this->transfers.pop();
    libusb_free_transfer(transfer);
  }
  while (!buffers.empty()){
    buffer = this->buffers.front();
    this->buffers.pop();
    delete(buffer);
  }
}

int Dionysus::set_comm_mode(){
  int retval = 0;
  retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_RESET);
    CHECK_ERROR("Failed to reset bitmode");
  retval = ftdi_set_latency_timer(this->ftdi, 2);
    CHECK_ERROR("Failed to set latency");
  retval = ftdi_usb_purge_buffers(this->ftdi);
    CHECK_ERROR("Failed to purge buffers");
  /*
  retval = ftdi_read_data_set_chunksize(this->ftdi, FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set read chunk size");
  retval = ftdi_write_data_set_chunksize(this->ftdi, FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set write chunk size");
  */
  /*
  retval = ftdi_setflowctrl(this->ftdi, SIO_DISABLE_FLOW_CTRL);
    CHECK_ERROR("Failed to set flow control to hw");
  Set hardware flow control
  retval = ftdi_setflowctrl(this->ftdi, SIO_RTS_CTS_HS);
    CHECK_ERROR("Failed to set flow control");
  */
  retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_SYNCFF);
    CHECK_ERROR("Failed to reset bitmode");
  retval = ftdi_usb_purge_buffers(this->ftdi);
    CHECK_ERROR("Failed to purge buffers");
  this->comm_mode = true;
  return 0;
}

int Dionysus::print_status(bool get_status, uint16_t in_status){
  //Get Control Status
  uint16_t status;
  uint8_t buffer[10];
  int retval = 0;
  //Control status transfer in from location
  printd("Entered\n");
  if (get_status){
    retval = libusb_control_transfer( this->ftdi->usb_dev,          // Context
                                      FTDI_DEVICE_IN_REQTYPE,       // Mix of VID:PID In Request
                                      SIO_POLL_MODEM_STATUS_REQUEST,// Command to execute
                                      0,                            // Value
                                      this->ftdi->index,            // Index
                                      &buffer[0],                   // uint8_t buffer
                                      2,                            // Size of read
                                      0);                           // Timeout
    if (retval != 2){
      printf ("Error reading status: 0x%02X\n", retval);
      return -1;
    }
    status = buffer[1] << 8 | buffer[0];
  }
  else {
    status = in_status;
  }
  retval = 0;

  printf ("Status: 0x%04X\n", status);
  if (status & MODEM_CTS){
    printf ("\tClear to send\n");
  }
  if (status & MODEM_DSR){
    printf ("\tData set ready\n");
  }
  if (status & MODEM_RI){
    printf ("\tRing indicator\n");
  }
  if (status & MODEM_RLSD) {
      printf ("\tCarrier detect\n");
  }
  if (status & MODEM_DR){
    printf ("\tData ready\n");
    retval = 1;
  }
  if (status &  MODEM_OE){
    printf ("\tOverrun error\n");
  }
  if (status &  MODEM_PE){
    printf ("\tParity error\n");
  }
  if (status &  MODEM_FE){
    printf ("\tFraming error\n");
  }
  if (status &  MODEM_BI){
    printf ("\tBreak interrupt\n");
  }
  if (status &  MODEM_THRE){
    printf ("\tTransmitter holding register\n");
  }
  if (status &  MODEM_TEMT){
    printf ("\tTransmitter empty\n");
    retval = 2;
  }
  if (status &  MODEM_RCVE){
    printf ("\tError in RCVR FIFO\n");
  }
  printf ("\n");
  return retval;
}
/* I/O */
static void dionysus_readstream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  uint8_t *buffer = transfer->buffer;
  uint32_t cpy_size = 0;
  uint16_t status = 0;
  int retval = 0;
  bool timeout;

  //printds("Entered\n");
  buf_size = transfer->actual_length;
  /*
  if (state->debug){
    printf("Requested Length: %d\n", transfer->length);
    printf("Actual length: %d\n", buf_size);
  }
  */
  gettimeofday(&state->timeout_now, NULL);
  //uint32_t timeout_val = (TimevalDiff(&state->timeout_now, &state->timeout_start) * 1000);
  //printf ("Timeout value: %d\n", timeout_val);
    
  timeout = (TimevalDiff(&state->timeout_now, &state->timeout_start) * 1000) > state->timeout;

  if ( timeout || 
      (transfer->status != LIBUSB_TRANSFER_COMPLETED) ||
      (state->error != 0)){

    if (timeout) {
      state->error = -10;
    }

    if (state->debug){

      //Error When reading from the transfer QUEUE
      //No more data to send, recover this transfer
      if (state->error == 0){
        printds ("Error conditions occured during transfer!\n");
        print_transfer_status(transfer);
      }
    }
    state->transfer_queue->push(transfer);
    if (state->transfer_queue->size() == NUM_TRANSFERS){
      //We're done!
      state->finished = true;
    }
    //Put the buffer back into the queue too
    state->buffer_queue->push(transfer->buffer);
    return;
  }

  //USB Transfer is good!

  //Waiting for the header
  if (buf_size <= 2){
    //printds("Small packet ( <= 2 )\n");
    //we didn't get data back, we need to submit a new one
    libusb_fill_bulk_transfer(transfer,
                              state->usb_dev,
                              state->out_ep,
                              transfer->buffer,
                              //BUFFER_SIZE + 2,
                              BUFFER_SIZE,
                              dionysus_readstream_cb,
                              state,
                              1000);
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    transfer->flags = 0;
    retval = libusb_submit_transfer(transfer);
    return;
  }
  //Buffer has more than the modem status
  //Go to the buffer position after the modem status
  buffer = &buffer[2];
  buf_size -= 2;

  /*
  printf ("Incomming buffer\n");
  for (int i = 0; i < buf_size; i++){
    printf ("%02X ", buffer[i]);
  }
  printf ("\n");
  */

  //Header Daata
  if (!state->header_found){
    printds("Reading header data\n");
    if (buf_size >= state->header_size){
      cpy_size = state->header_size;
    }
    else {
      cpy_size = buf_size;
    }
    memcpy(((uint8_t *)&state->response_header) + state->header_pos, buffer, cpy_size);
    state->header_pos += cpy_size;
    state->usb_actual_pos += cpy_size;
    if (state->header_pos >= state->header_size){
      //the response structure should be populated with header data
      //If this is a ping or a write then we are done, and we can exit immediately
      retval = check_response(state, &state->response_header);
      state->header_found = true;
    }
    buffer = &buffer[cpy_size];
    buf_size -= cpy_size;
  }

  //Buffer Data
  if ((buf_size > 0) && ((state->buffer_size - state->buffer_pos) > 0)){
    if (state->debug){
      printds("reading buffer data\n");
    }
    if (buf_size >= (state->buffer_size - state->buffer_pos)){
      //there is a chance the buffer size might be bigger than the data
      cpy_size = (state->buffer_size - state->buffer_pos);
    }
    else {
      //the incomming data is smaller or equal to the size of the data
      cpy_size = buf_size;
    }
    //Copy any remaining data to the output buffer
    memcpy(&state->buffer[state->buffer_pos], buffer, cpy_size);
    state->buffer_pos += cpy_size;
    state->usb_actual_pos += cpy_size;
    buf_size -= cpy_size;
  }

  //Calculate our USB position
  //state->usb_actual_pos += transfer->actual_length - 2; //Calculated above
  /*
  if (state->debug){
    printf ("Actual Position: (Dec) %d\n", state->usb_actual_pos);
    printf ("Actual Position: (Hex) 0x%08X\n", state->usb_actual_pos);
  }
  */
  if (state->usb_actual_pos < state->usb_total_size){
    //Because everything was sent in increments of chunksizes we need to see if the USB returned
    //something smaller, if so we might need to submit a new packet
    state->usb_pos = state->usb_pos - (transfer->length - transfer->actual_length);
    if (state->debug){
      printf ("request pos: 0x%08X, actual: 0x%08X\n", state->usb_pos, state->usb_actual_pos);
    }
    state->usb_size_left = state->usb_total_size - state->usb_pos;
    if (state->usb_size_left < 0){
      state->usb_size_left = 0;
    }
    if (state->debug){
      printf("%s(): Request %d more bytes from USB\n", __func__, state->usb_size_left);
    }
  }
  else {
    state->usb_size_left = 0;
  }

  //Check if we need to request more data
  if (state->usb_size_left > 0){
    printds("Submit a new transfer in read callback\n");
    libusb_fill_bulk_transfer(transfer,
                              state->usb_dev,
                              state->out_ep,
                              transfer->buffer,
                              //BUFFER_SIZE + 2, //Add two for the modem status
                              BUFFER_SIZE, //Add two for the modem status
                              dionysus_readstream_cb,
                              state,
                              1000);
    //Update the position and size left
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    transfer->flags = 0;
    retval = libusb_submit_transfer(transfer);
    if (retval != 0){
      printf ("Failed to submit transfer: %d\n", retval);
      //Put the transfer back into the empty queue
      state->transfer_queue->push(transfer);
      state->buffer_queue->push(buffer);
      state->d->cancel_all_transfers();
      state->error = retval;
    }
    state->usb_pos += BUFFER_SIZE - 2;
    state->usb_size_left = state->usb_total_size - BUFFER_SIZE - 2;
    if (state->usb_size_left < 0){
      state->usb_size_left = 0;
    }
  }
  else {
    //No need to submit a new USB Transfer
    printds("no need to submit a new transfer\n");
    if (state->debug){
      print_transfer_status(transfer);
    }
    state->transfer_queue->push(transfer);
    state->buffer_queue->push(transfer->buffer);
    state->error = 0;
  }

  //Check to see if we are done
  if (state->debug){
    printf ("Number of transfers: %d, transfers available: %d\n", ((int)NUM_TRANSFERS), (int)state->transfer_queue->size());
  }
  if (state->transfer_queue->size() == NUM_TRANSFERS){
    //All transfer queues are recovered!
    //We're done!
    state->finished = true;
  }
}

int Dionysus::read(uint32_t header_len, uint8_t *buffer, uint32_t size, uint32_t timeout){

  struct libusb_transfer * transfer;
  uint32_t buf_size = 0;
  uint8_t * buf = NULL;
  int retval = 0;

  printd("Entered\n");

  this->state->buffer           = buffer;
  this->state->buffer_pos       = 0;
  this->state->buffer_size      = size;

  //Total size that will be read from the chip
  this->state->usb_total_size   = size + header_len;
  //size of the read that is left
  this->state->usb_size_left    = 0;
  //current position of the request
  this->state->usb_pos          = 0;
  //current position of the data read back from the device
  this->state->usb_actual_pos   = 0;

  this->state->header_pos       = 0;

  this->state->error            = 0;
  this->state->header_found     = false;
  this->state->read_data_count  = 0;
  this->state->read_dev_addr    = 0;
  this->state->read_reg_addr    = 0;
  this->state->read_mem_addr    = 0;
  this->state->mem_response     = ((this->state->command_header.command & MEM_FLAG) > 0);
  this->state->finished         = false;
  this->state->timeout          = timeout;

  if (transfer_queue.empty()){
    printf ("Transfer queue empty!\n");
    return -5;
  }
  //setup a list of transfer_queue
  this->state->usb_size_left = this->state->usb_total_size;
  if (this->state->usb_total_size > 0){
    printd("Requesting data in the read\n");
    while (!transfer_queue.empty()){
      transfer = this->transfer_queue.front();
      buf = this->buffer_queue.front();
      if (this->debug){
        printf ("Buffer: %p\n", buf);
      }
      this->transfer_queue.pop();
      this->buffer_queue.pop();
      libusb_fill_bulk_transfer(transfer,
                                this->ftdi->usb_dev,
                                this->ftdi->out_ep,
                                buf,
                                //BUFFER_SIZE + 2,
                                BUFFER_SIZE,
                                //max_packet_size + 2, //Add two for the modem status

                                /*  Instead of only asking for the size we want we submit full packets
                                 *  this way if the FTDI chip only sends back modem status we can
                                 *  resubmit without having to figure out how large a packet should be send
                                 *  the FTDI chip will only send back the size that it has, so if we have a
                                 *  packet smaller than the maximum in it, we wont be waiting indefinetly
                                 * */
                                dionysus_readstream_cb,
                                this->state,
                                1000);

      transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
      transfer->flags = 0;
      printd ("Submit transfer\n");
      retval = libusb_submit_transfer(transfer);
      printd ("transfer submitted\n");
      if (retval != 0){
        //Clean up the USB stack by telling everything to cancel!
        //This will return everything back in the callback, so we still need to wait for it finish
        this->cancel_all_transfers();
        printf ("Error when submitting read transfer: %d\n", retval);
        this->state->error = -2;
        break;
      }

      this->state->usb_pos += BUFFER_SIZE - 2;
      this->state->usb_size_left = this->state->usb_total_size - this->state->usb_pos;
      if (this->state->usb_size_left < 0){
        this->state->usb_size_left = 0;
      }
      //Check if there is more data to write
      if (this->state->usb_size_left == 0){
        break;
      }
    }
  }
  while (!this->state->finished){
    retval = libusb_handle_events_completed(this->ftdi->usb_ctx, NULL);
    //printf(".");
  }
  if (this->state->error == 0) {
    return this->state->usb_total_size - this->state->usb_size_left;
  }
  else {
    return this->state->error;
  }
}

static void dionysus_writestream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  int retval = 0;
  bool timeout;
  printds ("Entered\n");
  gettimeofday(&state->timeout_now, NULL);
  timeout = (TimevalDiff(&state->timeout_now, &state->timeout_start) * 1000) > state->timeout;
  if (timeout||
      ((transfer->status != LIBUSB_TRANSFER_COMPLETED) ||
      (state->error != 0))){

    if (state->error == 0){
      if (timeout) {
        state->error = -10;
        printf ("Timeout while writing!\n");
      }
      if (state->debug){
        print_transfer_status(transfer);
      }
      state->d->cancel_all_transfers();
    }
    //Transfer Failed!
    state->error = -1;
    return;
  }
  state->transfer_queue->push(transfer);
  if (state->transfer_queue->size() == NUM_TRANSFERS){
    //We're Done
    printds("Finished!\n");
    state->finished = true;
  }
}

int Dionysus::write(uint32_t header_len, uint8_t *buffer, int size, uint32_t timeout){
  struct libusb_transfer * transfer;
  uint32_t buf_size = 0;
  uint8_t * buf = NULL;
  int retval = 0;
  uint32_t buffer_size_left = 0;

  printd ("Write transaction\n");
  retval = this->set_comm_mode();

  this->state->buffer           = buffer;
  this->state->buffer_pos       = 0;
  this->state->buffer_size      = size;

  //Total size that will be read from the chip
  this->state->usb_total_size   = size + header_len;
  //size of the read that is left
  this->state->usb_size_left    = 0;
  //current position of the request
  this->state->usb_pos          = 0;
  //current position of the data read back from the device
  this->state->usb_actual_pos   = 0;

  this->state->header_pos       = 0;
  this->state->header_size      = header_len;

  this->state->error            = 0;
  this->state->header_found     = false; // this isn't needed for a write
  this->state->finished         = false;
  this->state->timeout          = timeout;
  gettimeofday(&this->state->timeout_start, NULL);

  //Setup a list of transfers
  if (transfer_queue.empty()){
    printf ("Transfer queue empty!\n");
    return -5;
  }

  //Send the header
  if (header_len > 0){
    if (this->debug){
      printd ("Header:\n");
      for (int i = 0; i < header_len; i++){
        printf ("%02X ", ((uint8_t *) &this->state->command_header)[i]);
      }
      printf ("\n");
    }
    transfer = transfer_queue.front();
    transfer_queue.pop();
    //printf ("Transfer Length: %d\n", header_len);
    libusb_fill_bulk_transfer(transfer,
                            this->ftdi->usb_dev,
                            this->ftdi->in_ep,
                            (uint8_t *) &this->state->command_header,
                            header_len,
                            dionysus_writestream_cb,
                            this->state,
                            1000);
    transfer->flags = 0;
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    retval = libusb_submit_transfer(transfer);
    printd("Submitted header transfer\n");
    if (retval != 0){
      //XXX: Need a way to clean up the USB stack
      printf ("Error when submitting write transfer: %d\n", retval);
      this->cancel_all_transfers();
      this->state->error = -3;
    }
    this->state->usb_pos += header_len;
  }
  if (size > 0){
    transfer = transfer_queue.front();
    transfer_queue.pop();
    libusb_fill_bulk_transfer(transfer,
                              this->ftdi->usb_dev,
                              this->ftdi->in_ep,
                              buffer,
                              size,
                              dionysus_writestream_cb,
                              this->state,
                              1000);
    transfer->flags = 0;
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    retval = libusb_submit_transfer(transfer);
    printd("Submitted data transfer\n");
    if (retval != 0){
      printf ("Error when submitting write transfer: %d\n", retval);
    }
  }
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
    retval = libusb_handle_events_completed(this->ftdi->usb_ctx, NULL);
    if (this->debug){
      //printf (".");
    }
  }
  //retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_RESET);
  if (this->debug){
    printf ("Finished %d left of %d\n", this->state->usb_size_left, size);
  }
  //this->print_status(true, 0);
  return size - this->state->usb_size_left;
}

int Dionysus::write_sync(uint8_t *buffer, uint16_t size){
  int retval;
  //this->print_status(true, 0);

  retval = ftdi_write_data(this->ftdi, buffer, size);
  //printf ("Retval: %d\n", retval);
  //this->print_status(true, 0);
  return 0;
}

int Dionysus::read_sync(uint8_t *buffer, uint16_t size){
  int retval;
  //this->print_status(true, 0);
  retval = ftdi_read_data(this->ftdi, buffer, size);
  /*
  printf ("Retval: %d\n", retval);
  printf ("Buffer:\n");
  for (int i = 0; i < size; i++){
    printf ("%02X ", buffer[i]);
  }
  printf ("\n");
  */
  return 0;
}

void Dionysus::cancel_all_transfers(){
  //Get the status of each transfer
  //If it is still in progress cancel it
  struct libusb_transfer * transfer;
  uint32_t count = this->transfers.size();
  printd("Entered\n");
  //Go through each item in the transfer
  for (int i = 0; i < count; i++){
    transfer = this->transfers.front();
    this->transfers.pop();
    this->transfers.push(transfer);
    //This will return an error if the transfer has already been
    //cancelled or finished but we can safely ignore it
    libusb_cancel_transfer(transfer);
  }
}

