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
    LIBUSB_TRANSFER_COMPLETED:
      printf ("Transfer Completed\n");
      break;
    LIBUSB_TRANSFER_ERROR:
      printf ("Transfer Error\n");
      break;
    LIBUSB_TRANSFER_TIMED_OUT:
      printf ("Transfer Timed Out\n");
      break;
    LIBUSB_TRANSFER_CANCELLED:
      printf ("Transfer Cancelled\n");
      break;
    LIBUSB_TRANSFER_STALL:
      printf ("Transfer Stalled\n");
      break;
    LIBUSB_TRANSFER_NO_DEVICE:
      printf ("Transfer No Device\n");
      break;
    LIBUSB_TRANSFER_OVERFLOW:
      printf ("Transfer Overflow\n");
      break;
    default:
      break;
  }
}

/* I/O */
static void dionysus_readstream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  int retval = 0;

  printds("Entered\n");
  if ((transfer->status == LIBUSB_TRANSFER_COMPLETED) && state->error == 0){
    if (!state->header_found){
      //the response structure should be populated with header data
      //If this is a ping or a write then we are done, and we can exit immediately
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
    if (state->error == 0){
      printf ("Unknown USB Return Status: %d\n", transfer->status);
    }
    //Recover the transfer
    state->transfer_queue->push(transfer);
    if (state->transfer_queue->size() == NUM_TRANSFERS){
      //We're done!
      state->finished = true;
    }

    state->error = -1;
  }
}

void Dionysus::usb_constructor(){
  //Initialize the read structure
  this->state = new state_t;
  this->state->transfer_index = 0;
  this->state->f = NULL;
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

void Dionysus::usb_destructor(){
  struct libusb_transfer *transfer = NULL;
  uint8_t * buffer = NULL;
  //Empty the working queue
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
    delete(transfer);
  }
  while (!buffers.empty()){
    buffer = this->buffers.front();
    this->buffers.pop();
    delete(buffer);
  }
}

int Dionysus::read(uint32_t header_len, uint8_t *buffer, uint32_t size){

  int max_packet_size = this->state->f->max_packet_size;
  struct libusb_transfer * transfer;
  //int packets_per_transfer = PACKETS_PER_TRANSFER;
  //4096 is the buffer size we set inside the FTDI Chip so we are looking to fill that up
  int packets_per_transfer = FTDI_BUFFER_SIZE / max_packet_size;
  uint32_t buf_size = 0;
  int retval = 0;

  uint32_t max_buf_size = packets_per_transfer * max_packet_size;
  //Go into reset to set everything up before the first transaction otherwise the SYNC FF
  //will stutter
  printd("Entered\n");
  //retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_RESET);
  //  CHECK_ERROR("Failed to reset Bitmode");
  //Purge the buffers
  //retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
  //  CHECK_ERROR("Failed to purge buffers");

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
    printf ("Length of transfer queue: %ld\n", transfer_queue.size());
    transfer = transfer_queue.front();
    transfer_queue.pop();
    print_transfer_status(transfer);
    libusb_fill_bulk_transfer(transfer,
                              this->state->f->usb_dev,
                              this->state->f->out_ep,
                              (uint8_t *) &this->state->response_header,
                              header_len,
                              dionysus_readstream_cb,
                              this->state,
                              10000);
    print_transfer_status(transfer);
    retval = libusb_submit_transfer(transfer);
    print_transfer_status(transfer);
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
      transfer_queue.pop();
      print_transfer_status(transfer);
      libusb_fill_bulk_transfer(transfer,
                                this->state->f->usb_dev,
                                this->state->f->out_ep,
                                &buffer[this->state->pos],
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

  printd("Finished Assembling packets\n");
  //retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_SYNCFF);
  //if (retval != 0){
    //XXX: Need a way to clean up the usb stack
  //  CHECK_ERROR("Failed to set Synchronous FIFO, Critical ErrRRrRRooOOoorRRRR!");
  //}
  retval = this->Ftdi::Context::set_bitmode(0x00, BITMODE_SYNCFF);
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
    //print_transfer_status(transfer);
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
  int max_packet_size = this->state->f->max_packet_size;
  struct libusb_transfer * transfer;
  int packets_per_transfer = FTDI_BUFFER_SIZE / max_packet_size;
  uint32_t buf_size = 0;
  int retval = 0;

  uint32_t max_buf_size = packets_per_transfer * max_packet_size;

  printd ("Write transaction\n");
  //Set reset so that we don't run into a stutter mode
  //retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_RESET);
  //  CHECK_ERROR("Failed to reset Bitmode");
  //Purge the buffers
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");

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
    libusb_fill_bulk_transfer(transfer,
                            this->state->f->usb_dev,
                            this->state->f->in_ep,
                            (uint8_t *) &this->state->command_header,
                            header_len,
                            dionysus_writestream_cb,
                            this->state,
                            0);
    retval = libusb_submit_transfer(transfer);
    if (retval != 0){
      //XXX: Need a way to clean up the USB stack
      printf ("Error when submitting write transfer: %d\n", retval);
      this->state->error = -3;
      this->cancel_all_transfers();
    }
  }

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

  retval = this->Ftdi::Context::set_bitmode(0x00, BITMODE_SYNCFF);
  if (retval != 0){
    this->state->error = -4;
    this->cancel_all_transfers();
    printf ("Failed to set Synchronous FIFO, Critical ERRorRORoooRooRR!\n");
    printf ("Cancelling all transfers\n");
  }
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
    if (this->debug){
      printf (".");
    }
  }
  if (this->debug){
    printf ("Finished %d left of %d\n", this->state->size_left, size);
  }
  return size - this->state->size_left;
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

