#include "dionysus_local.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>

//Constructor
Dionysus::Dionysus(bool debug) : Ftdi::Context(){
  //Open up Dionysus
  this->debug = debug;
  this->comm_mode = false;
  this->usb_constructor();
  if (this->debug) printf ("Dionysus: Debug Enabled\n");
}

Dionysus::~Dionysus(){
  //Close Dionysus
  if (this->is_open()) {
    if (this->debug) printf ("Dionysus: FTDI is open, close it\n");
    this->close();
  }
  this->usb_destructor();
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
  this->reset();
  this->Ftdi::Context::set_bitmode(0xFF, BITMODE_RESET);
    CHECK_ERROR("Failed to go into bitmode reset");
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
  retval = this->Ftdi::Context::set_flow_control(SIO_RTS_CTS_HS);
    CHECK_ERROR("Failed to set flow control");
  //Clear the buffers to flush things out
  retval = this->Ftdi::Context::flush(Context::Input | Context::Output);
    CHECK_ERROR("Failed to purge buffers");

  this->state->f = this->context();
  //libusb_set_debug(this->state->f->usb_ctx, 3);

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

int Dionysus::soft_reset(){
  return this->strobe_pin((unsigned char)RESET_BUTTON);
}

int Dionysus::program_fpga(){
  return this->strobe_pin((unsigned char)PROGRAM_BUTTON);
}


