#include "dionysus.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>


struct _state_t {

  std::queue<struct libusb_transfer *> *transfer_queue;
  //Context of FTDI to continue transactions
  struct ftdi_context * f;
  uint32_t transfer_index;

  //Dionysus Memory Buffer used to store data
  uint8_t * buffer;
  uint32_t size_left;
  uint32_t size;
  uint32_t pos;

  //Reference to the class
  Dionysus *d;

  bool finished;
  int error;
};

//Constructor
Dionysus::Dionysus(bool debug) : Ftdi::Context(){
  //Open up Dionysus
  this->debug = debug;
  if (this->debug) printf ("Dionysus: Debug Enabled\n");
  if (this->debug) printf ("Dionysus: Create an FTDI Context\n");
  this->comm_mode = false;

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
  this->state->d = this;

  for (int i = 0; i < NUM_TRANSFERS; i++){
    struct libusb_transfer *transfer = NULL;
    transfer = libusb_alloc_transfer(0);
    transfer_queue.push(transfer);
    if (!transfer){
      printf ("Dionysus: Error while creating index %d usb transfer\n", i);
    }
  }
}


Dionysus::~Dionysus(){
  //Close Dionysus
  struct libusb_transfer *transfer = NULL;
  if (this->is_open()) {
    if (this->debug) printf ("Dionysus: FTDI is open, close it\n");
    this->close();
  }
  //Remove all the transfer_queue
  while (!transfer_queue.empty()){
    transfer = this->transfer_queue.front();
    this->transfer_queue.pop();
    libusb_free_transfer(transfer);
  }
  this->state->transfer_queue = NULL;
  delete(this->state);
}

//Properties
int Dionysus::open(int vendor, int product){
  int retval;
  this->vendor = vendor;
  this->product = product;

  if (this->debug) printf ("Dionysus: Open a context\n");
  retval = this->Ftdi::Context::open(vendor, product);
    CHECK_ERROR("Failed to open FTDI");
  //flush the buffers
  if (this->debug) printf ("\tFlush the buffer\n");
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");
  //Set Latency to 2
  if (this->debug) printf ("\tSet the latency\n");
  retval = this->Ftdi::Context::set_latency(2);
    CHECK_ERROR("Failed to set latency");
  //Set Asynchronous Mode so that we are in Control Mode (not comm)
  //Set Chunksize for both input and output to 4096 (max size)
  if (this->debug) printf ("\tSet the read chunksize to %d\n", FTDI_BUFFER_SIZE);
  retval = this->Ftdi::Context::set_read_chunk_size(FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set read chunk size");
  if (this->debug) printf ("\tSet the write chunksize to %d\n", FTDI_BUFFER_SIZE);
  retval = this->Ftdi::Context::set_write_chunk_size(FTDI_BUFFER_SIZE);
    CHECK_ERROR("Failed to set write chunk size");
  //Set hardware flow control
  //XXX: Do I need flow control?
  //Clear the buffers to flush things out
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");

  this->state->f = this->context();
  return retval;
}
bool Dionysus::is_open(){
  return this->Ftdi::Context::is_open();
}
int Dionysus::close(){
  if (this->debug) printf ("Dionysus: Close FTDI\n");
  return this->Ftdi::Context::close();
}
int Dionysus::reset(){
  if (this->debug) printf ("Dionysus: Reset FTDI\n");
  return this->Ftdi::Context::reset();
}

/* I/O */
static void ftdi_readstream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  int retval = 0;

  if ((transfer->status == LIBUSB_TRANSFER_COMPLETED) && state->error == 0){
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
                                &ftdi_readstream_cb,
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

int Dionysus::read(uint8_t *buffer, uint32_t size){

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
  retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_RESET);
    CHECK_ERROR("Failed to reset Bitmode");
  //Purge the buffers
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");

  this->state->buffer = buffer;
  this->state->pos = 0;
  this->state->size_left = size;
  this->state->size = size;
  this->state->error = 0;
  //setup a list of transfer_queue
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
    libusb_fill_bulk_transfer(transfer,
                              this->state->f->usb_dev,
                              this->state->f->out_ep,
                              &buffer[this->state->pos],
                              buf_size,
                              &ftdi_readstream_cb,
                              this->state,
                              0);
    this->state->pos += buf_size;

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

  retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_SYNCFF);
  if (retval != 0){
    //XXX: Need a way to clean up the usb stack
    CHECK_ERROR("Failed to set Synchronous FIFO, Critical ErrRRrRRooOOoorRRRR!");
  }
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
  }


  return size - this->state->size_left;
}

static void ftdi_writestream_cb(struct libusb_transfer *transfer){
  state_t * state = (state_t *) transfer->user_data;
  uint32_t buf_size = 0;
  int retval = 0;

  if ((transfer->status == LIBUSB_TRANSFER_COMPLETED) && state->error == 0){
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
                                &ftdi_writestream_cb,
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
      state->finished = true;
    }
    state->error = -1;
  }
}
int Dionysus::write(unsigned char *buffer, int size){
  int max_packet_size = this->state->f->max_packet_size;
  struct libusb_transfer * transfer;
  int packets_per_transfer = FTDI_BUFFER_SIZE / max_packet_size;
  uint32_t buf_size = 0;
  int retval = 0;

  uint32_t max_buf_size = packets_per_transfer * max_packet_size;

  //Set reset so that we don't run into a stutter mode
  retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_RESET);
    CHECK_ERROR("Failed to reset Bitmode");
  //Purge the buffers
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");

  this->state->buffer = buffer;
  this->state->pos = 0;
  this->state->size_left = size;
  this->state->size = size;
  this->state->error = 0;
  //Setup a list of transfers
  while (!transfer_queue.empty()){
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
                              &ftdi_writestream_cb,
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

  retval = this->Ftdi::Context::set_bitmode(0xFF, BITMODE_SYNCFF);
  if (retval != 0){
    this->state->error = -4;
    this->cancel_all_transfers();
    printf ("Failed to set Synchronous FIFO, Critical ERRorRORoooRooRR!\n");
    printf ("Cancelling all transfers\n");
  }
  while (!this->state->finished){
    //XXX: How to wait for an interrupt and then finish this function?
  }
  return size - this->state->size_left;
}

//Bit Control
int Dionysus::strobe_pin(unsigned char pin){
  unsigned char pins;
  unsigned char pin_command[1];
  int retval = 0;

  if (this->debug) printf ("Strobe signal\n");
  retval = this->Ftdi::Context::set_bitmode(0x00, BITMODE_BITBANG);
    CHECK_ERROR("Failed to reset Bitmode");

  ftdi_context *bb_ftdi;
  bb_ftdi = ftdi_new();
  if (bb_ftdi == NULL){
    if (this->debug) printf ("Failed to get Bitbang FTDI Port\n");
    return -3;
  }
  retval = ftdi_set_interface(bb_ftdi, INTERFACE_B);
    CHECK_ERROR("Failed to set interface to B");
  retval = ftdi_usb_open(bb_ftdi, this->vendor, this->product);
    CHECK_ERROR("Failed to open up new FTDI USB Context of interface B");
  //retval = ftdi_set_bitmode(bb_ftdi, (unsigned char) BUTTON_BITMASK, BITMODE_BITBANG);
  if (this->debug) printf ("Button Mask: 0x%02X\n", (unsigned char) BUTTON_BITMASK);
  retval = ftdi_set_bitmode(bb_ftdi, (unsigned char) BUTTON_BITMASK, BITMODE_BITBANG);
    CHECK_ERROR("Failed to set bitmode");

  retval = ftdi_read_pins(bb_ftdi, &pins);
    CHECK_ERROR("Failed to read pins");
  pin_command[0] = (pins | pin);
  //pin_command[0] = (pins | 0xFF);
  if (this->debug) {
    printf ("Write Buttons Values: 0x%02X\n", pin_command[0]);
  }

  retval = ftdi_write_data(bb_ftdi, pin_command, 1);
    CHECK_ERROR("Failed to write data to BB Mode pins");
  //Sleep for 300 ms
  usleep(300000);
  pin_command[0] = (pins & ~pin);
  //pin_command[0] = (0x00);
  if (this->debug) {
    printf ("Write Buttons Values: 0x%02X\n", pin_command[0]);
  }
  retval = ftdi_write_data(bb_ftdi, pin_command, 1);
    CHECK_ERROR("Failed to write data to BB Mode pins");
  retval = ftdi_disable_bitbang(bb_ftdi);
    CHECK_ERROR("Failed to disable bitbang mode");
  ftdi_usb_close(bb_ftdi);
  ftdi_free(bb_ftdi);
  this->comm_mode = false;
  return 0;
}
int Dionysus::soft_reset(){
  return this->strobe_pin((unsigned char)RESET_BUTTON);
}

int Dionysus::program_fpga(){
  return this->strobe_pin((unsigned char)PROGRAM_BUTTON);
}

void Dionysus::cancel_all_transfers(){
  //Get the status of each transfer
  //If it is still in progress cancel it
  struct libusb_transfer * transfer;
  uint32_t count = this->transfers.size();
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
