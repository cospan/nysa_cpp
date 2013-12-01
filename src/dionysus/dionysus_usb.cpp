#include "dionysus_local.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>


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
  this->state = new state_t;
  this->state->transfer_index = 0;
  this->state->f = this->ftdi;
  this->state->finished = true;
  this->state->error = 0;
  this->state->buffer = NULL;
  this->state->size_left = 0;
  this->state->size = 0;
  this->state->pos = 0;
  this->state->transfer_queue = &this->transfer_queue;
  this->state->buffer_queue = &this->buffer_queue;
  this->state->d = this;
  this->state->debug = debug;
  this->state->read_data_count = 0;
  this->state->read_dev_addr = 0;
  this->state->read_reg_addr = 0;
  this->state->read_mem_addr = 0;
  this->state->mem_response = false;

  for (int i = 0; i < NUM_TRANSFERS; i++){
    uint8_t * buffer = new uint8_t[FTDI_BUFFER_SIZE];
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
  this->usb_is_open = true;
  retval = Dionysus::set_comm_mode();
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
  retval = ftdi_read_data_set_chunksize(this->ftdi, FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set read chunk size");
  retval = ftdi_write_data_set_chunksize(this->ftdi, FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set write chunk size");
  /*
  retval = ftdi_setflowctrl(this->ftdi, SIO_DISABLE_FLOW_CTRL);
    CHECK_ERROR("Failed to set flow control to hw");
  Set hardware flow control
  retval = this->Ftdi::Context::set_flow_control(SIO_RTS_CTS_HS);
    CHECK_ERROR("Failed to set flow control");
  Clear the buffers to flush things out
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");
  */

  /*
  retval = libusb_control_transfer( this->state->f->usb_dev,      // Context
                                    FTDI_DEVICE_OUT_REQTYPE,      // Mix of VID:PID out Request
                                    SIO_SET_FLOW_CTRL_REQUEST,    // Command to Execute
                                    (SIO_RTS_CTS_HS | INTERFACE_A),   // Value
                                    1,                            // Index
                                    NULL,                         // uint8_t buffer
                                    0,                            // Size of read
                                    0);                           // Timeout
                                    */
  retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_SYNCFF);
    CHECK_ERROR("Failed to reset bitmode");
  //printf ("Index: 0x%02X\n", state->f->index);
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
  uint16_t status = 0;
  int retval = 0;

  printds("Entered\n");
  printf("Actual length: %d\n", transfer->actual_length);
  if ((transfer->status == LIBUSB_TRANSFER_COMPLETED) && state->error == 0){
    if (!state->header_found){
      if (transfer->actual_length <= 2){
        libusb_fill_bulk_transfer(transfer,
                                  state->f->usb_dev,
                                  state->f->out_ep,
                                  (uint8_t *) &state->response_header,
                                  RESPONSE_HEADER_LEN + 2,
                                  //this->ftdi->readbuffer_chunksize,
                                  dionysus_readstream_cb,
                                  state,
                                  1000);
        transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
        transfer->flags = 0;
        retval = libusb_submit_transfer(transfer);
        return;
      }
      state->buffer_queue->push(transfer->buffer);
      memcpy((uint8_t *) &state->response_header, buffer, RESPONSE_HEADER_LEN);
      //the response structure should be populated with header data
      //If this is a ping or a write then we are done, and we can exit immediately
      printf ("Incomming buffer\n");
      //XXX: state->d->print_status(false, state->response_header.modem_status);
      //for (int i = 0; i < RESPONSE_HEADER_LEN; i++){
      for (int i = 0; i < 512; i++){
        printf ("%02X ", buffer[i]);
      }
      printf ("\n");
      if (state->response_header.id != ID_RESPONSE){
        //Fail, ID does not match
        printf ("ID Response != 0x%0X: %02X\n", ID_RESPONSE, state->response_header.id);
        //state->d->cancel_all_transfers();
      }
      //XXX: how to check the command response
      else if (state->response_header.status != ~state->command_header.command){
        printf ("Status Read != ~Command\n");
        //state->d->cancel_all_transfers();
      }
      if (state->debug){
        printf ("~status: 0x%02X\n", ~state->response_header.status);
      }
      state->read_data_count =  state->response_header.data_count[2] << 16;
      state->read_data_count |= state->response_header.data_count[1] << 8;
      state->read_data_count |= state->response_header.data_count[0];
      if (state->debug){
        printf ("Data Count (32-bit Data words): 0x%02X\n", state->read_data_count);
      }

      if (state->mem_response){
        state->read_mem_addr =  state->response_header.address.mem_addr[3] << 24;
        state->read_mem_addr |= state->response_header.address.mem_addr[2] << 16;
        state->read_mem_addr |= state->response_header.address.mem_addr[1] << 8;
        state->read_mem_addr |= state->response_header.address.mem_addr[0];
        if (state->debug){
          printf ("Memory Access @ 0x%08X\n", state->read_mem_addr);
        }
      }
      else {
        state->read_dev_addr = state->response_header.address.dev_addr;
        state->read_reg_addr = state->response_header.address.reg_addr[2] << 16;
        state->read_reg_addr |= state->response_header.address.reg_addr[1] << 8;
        state->read_reg_addr |= state->response_header.address.reg_addr[0];
        if (state->debug){
          printf ("Peripheral Access: Device: 0x%02X Register: 0x%02X\n", state->read_dev_addr, state->read_reg_addr);
        }
      }

      state->header_found = true;
      if (state->size_left == 0){
        state->transfer_queue->push(transfer);
      }
    }
    else {
      //Not Header Data
      if (state->size_left > 0){
        //Still more data to send
        if (state->size_left > state->f->max_packet_size){
          //Can't send all the data yet
          buf_size = state->f->max_packet_size;
        }
        else {
          //Sending the reset of the data
          buf_size = state->size_left;
        }
        libusb_fill_bulk_transfer(transfer,
                                  state->f->usb_dev,
                                  state->f->out_ep,
                                  &state->buffer[state->pos],
                                  buf_size,
                                  dionysus_readstream_cb,
                                  state,
                                  0);
        //Update the position and size left
        state->size_left -= buf_size;
        state->pos += buf_size;
        retval = libusb_submit_transfer(transfer);
        if (retval != 0){
          printf ("Failed to submit transfer: %d\n", retval);
          //Put the transfer back into the empty queue
          state->transfer_queue->push(transfer);
          state->error = retval;
        }
      }
      else {
        //No more data to send, recover this transfer
        state->transfer_queue->push(transfer);
        if (state->transfer_queue->size() == NUM_TRANSFERS){
          //We're done!
          state->finished = true;
        }
      }
    }
  }
  else {
    //if (state->error == 0){
    print_transfer_status(transfer);
    //  printf ("Unknown USB Return Status: %d\n", transfer->status);
    //}
    //Recover the transfer
    state->transfer_queue->push(transfer);
    state->buffer_queue->push(transfer->buffer);
    state->error = -1;
  }
  if (state->transfer_queue->size() == NUM_TRANSFERS){
    //We're done!
    state->finished = true;
  }

}

int Dionysus::read(uint32_t header_len, uint8_t *buffer, uint32_t size){

  int max_packet_size = this->ftdi->max_packet_size;
  struct libusb_transfer * transfer;
  //int packets_per_transfer = PACKETS_PER_TRANSFER;
  //4096 is the buffer size we set inside the FTDI Chip so we are looking to fill that up
  int packets_per_transfer = FTDI_BUFFER_SIZE / max_packet_size;
  uint32_t buf_size = 0;
  uint8_t * buf = NULL;
  int retval = 0;

  uint32_t max_buf_size = packets_per_transfer * max_packet_size;
  //Go into reset to set everything up before the first transaction otherwise the SYNC FF
  //will stutter
  printd("Entered\n");
  //retval = ftdi_set_bitmode(this->ftdi, 0xFF, BITMODE_RESET);
  //  CHECK_ERROR("Failed to reset Bitmode");

  this->state->buffer          = buffer;
  this->state->pos             = 0;
  this->state->size_left       = size;
  this->state->size            = size;
  this->state->error           = 0;
  this->state->header_found    = false;
  this->state->read_data_count = 0;
  this->state->read_dev_addr   = 0;
  this->state->read_reg_addr   = 0;
  this->state->read_mem_addr   = 0;
  this->state->mem_response    = ((this->state->command_header.command & MEM_FLAG) > 0);
  this->state->finished        = false;

  if (transfer_queue.empty()){
    printf ("Transfer queue empty!\n");
    return -5;
  }
  if (header_len > 0){
    printd ("Sending header\n");
    //printf ("Length of transfer queue: %ld\n", transfer_queue.size());
    transfer = transfer_queue.front();
    buf = buffer_queue.front();
    transfer_queue.pop();
    buffer_queue.pop();
    //print_transfer_status(transfer);
    libusb_fill_bulk_transfer(transfer,
                              this->ftdi->usb_dev,
                              this->ftdi->out_ep,
                              (uint8_t *) &this->state->response_header,
                              header_len + 2,
                              //this->ftdi->readbuffer_chunksize,
                              dionysus_readstream_cb,
                              this->state,
                              1000);
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    transfer->flags = 0;
    retval = libusb_submit_transfer(transfer);
    if (retval != 0){
      printf ("Error when submitting read transfer: %d\n", retval);
      this->state->error = -2;
    }
    printd ("header transfer submitted\n");
  }

  //setup a list of transfer_queue
  if (size > 0){
    printd("Requesting data in the read\n");
    while (!transfer_queue.empty()){
      this->state->size_left = size - this->state->pos;
      if (this->state->size_left >= max_packet_size){
        //The packet size is larger or equal to than the size of the BULK transfer packet
        buf_size = max_packet_size;
        this->state->size_left -= buf_size;
      }
      else {
        //The packet size is smaller than the size of the BULK transfer packet
        buf_size = this->state->size_left;
        this->state->size_left = 0;
      }
      transfer = transfer_queue.front();
      buf = buffer_queue.front();
      transfer_queue.pop();
      buffer_queue.pop();
      print_transfer_status(transfer);
      libusb_fill_bulk_transfer(transfer,
                                this->state->f->usb_dev,
                                this->state->f->out_ep,
                                &state->buffer[state->pos],
                                buf_size,
                                dionysus_readstream_cb,
                                this->state,
                                1000);
      print_transfer_status(transfer);
      this->state->pos += buf_size;

      retval = libusb_submit_transfer(transfer);
      if (retval != 0){
        //XXX: Need a way to clean up the USB stack
        printf ("Error when submitting read transfer: %d\n", retval);
        this->state->error = -2;
        break;
      }

      //Check if there is more data to write
      if (this->state->size_left == 0){
        break;
      }
    }
  }
  //retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_SYNCFF);
  //printd("Wait for read to finish\n");
  while (!this->state->finished){
    retval = libusb_handle_events_completed(this->ftdi->usb_ctx, NULL);
    printf(".");
  }
  return size - this->state->size_left;
}

static void dionysus_writestream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  int retval = 0;
  printds ("Entered\n");
  if ((transfer->status == LIBUSB_TRANSFER_COMPLETED) && state->error == 0){
    //printf ("Transfer Completed with Status: 0x%02X\n", transfer->status);
    if (state->size_left > 0){
      //more data to send
      if (state->size_left > state->f->max_packet_size){
        buf_size = state->f->max_packet_size;
      }
      else {
        //Sending the rest of the data
        buf_size = state->size_left;
      }

      libusb_fill_bulk_transfer(transfer,
                                state->f->usb_dev,
                                state->f->in_ep,
                                &state->buffer[state->pos],
                                buf_size,
                                dionysus_writestream_cb,
                                state,
                                0);
      state->size_left -= buf_size;
      state->pos += buf_size;
      retval = libusb_submit_transfer(transfer);
      if (retval != 0){
        printf ("Failed to submit transfer: %d\n", retval);
        //Put the transfer back into the empty queue
        state->transfer_queue->push(transfer);
        state->error = retval;
        state->d->cancel_all_transfers();
      }
    }
    else {
      //No more data to send, recover this transfer
      state->transfer_queue->push(transfer);
      if (state->transfer_queue->size() == NUM_TRANSFERS){
        //We're Done
        printds("Finished!\n");
        state->finished = true;
      }
    }
  }
  else {
    if (state->error == 0){
      printf ("Unknown USB Return Status: %d\n", transfer->status);
      state->d->cancel_all_transfers();
    }
    //Transfer Failed!
    state->transfer_queue->push(transfer);
    if (state->transfer_queue->size() == NUM_TRANSFERS){
      //We're done!
      printds("Finished!\n");
      state->finished = true;
    }
    state->error = -1;
  }
}

int Dionysus::write(uint32_t header_len, uint8_t *buffer, int size){
  int max_packet_size = this->ftdi->max_packet_size;
  struct libusb_transfer * transfer;
  int packets_per_transfer = FTDI_BUFFER_SIZE / max_packet_size;
  uint32_t buf_size = 0;
  uint8_t * buf = NULL;
  int retval = 0;

  uint32_t max_buf_size = packets_per_transfer * max_packet_size;

  printd ("Write transaction\n");
  retval = this->set_comm_mode();

  this->state->buffer = buffer;
  this->state->pos = 0;
  this->state->size_left = size;
  this->state->size = size;
  this->state->error = 0;
  this->state->header_found = false; // this isn't needed for a write
  this->state->finished = false;
  //Setup a list of transfers
  if (transfer_queue.empty()){
    printf ("Transfer queue empty!\n");
    return -5;
  }
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
                            header_len + 2,
                            dionysus_writestream_cb,
                            this->state,
                            0);
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    retval = libusb_submit_transfer(transfer);
    if (retval != 0){
      //XXX: Need a way to clean up the USB stack
      printf ("Error when submitting write transfer: %d\n", retval);
      this->state->error = -3;
      this->cancel_all_transfers();
    }
  }
  //Send the data in this write transfer
  while (!transfer_queue.empty() && (this->state->error == 0) && (size > 0)){
    this->state->size_left = size - this->state->pos;
    if (this->state->size_left >= max_packet_size){
      //Packet size is larger than the maximum packet size I can send to USB
      buf_size = max_packet_size;
      this->state->size_left -= buf_size;
    }
    else {
      //Packet size is smaller than the size of a BULK transfer packet
      buf_size = this->state->size_left;
      this->state->size_left = 0;
    }
    transfer = transfer_queue.front();
    transfer_queue.pop();
    libusb_fill_bulk_transfer(transfer,
                              this->state->f->usb_dev,
                              this->state->f->in_ep,
                              &buffer[this->state->pos],
                              buf_size,
                              dionysus_writestream_cb,
                              this->state,
                              0);
    this->state->pos += buf_size;
    retval = libusb_submit_transfer(transfer);
    if (retval != 0){
      //XXX: Need a way to clean up the USB stack
      printf ("Error when submitting write transfer: %d\n", retval);
      this->state->error = -3;
      this->cancel_all_transfers();
      break;
    }
    //Check if we have filld all the data in the buffer
    if (this->state->size_left == 0){
      break;
    }
  }
  printd ("Setting Synchronous FIFO\n");
  retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_SYNCFF);
    CHECK_ERROR("Failed set bitmode to Synchronous FIFO");

  if (retval != 0){
    this->state->error = -4;
    this->cancel_all_transfers();
    printf ("Failed to set Synchronous FIFO, Critical ERRorRORoooRooRR!\n");
    printf ("Cancelling all transfers\n");
  }
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
    if (this->debug){
      //printf (".");
    }
  }
  //retval = ftdi_set_bitmode(this->ftdi, 0x00, BITMODE_RESET);
  if (this->debug){
    printf ("Finished %d left of %d\n", this->state->size_left, size);
  }
  //this->print_status(true, 0);
  return size - this->state->size_left;
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

