#include "dionysus_local.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>

//Constructor
Dionysus::Dionysus(bool debug){
  //Open up Dionysus
  this->debug = debug;
  this->usb_is_open = false;
  this->comm_mode = false;
  this->usb_constructor();
  if (this->debug) printf ("Dionysus: Debug Enabled\n");
  //Open up a context and initialize it
  this->ftdi = ftdi_new();
  int retval = 0;
  retval = ftdi_init(this->ftdi);
  if (retval != 0){
    printf ("Error initializing FTDI Context\n");
  }
}
Dionysus::~Dionysus(){
  //Close Dionysus
  if (this->usb_is_open) {
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
  Dionysus::usb_open(vendor, product);

  //libusb_set_debug(this->state->f->usb_ctx, 3);
  return retval;
}
bool Dionysus::is_open(){
  return this->usb_is_open;
}
int Dionysus::close(){
  if (this->debug) printf ("Dionysus: Close FTDI\n");
  this->usb_is_open = false;
  return ftdi_usb_close(this->ftdi);
}
int Dionysus::reset(){
  if (this->debug) printf ("Dionysus: Reset FTDI\n");
  return ftdi_usb_reset(this->ftdi);
}
int Dionysus::soft_reset(){
  return this->strobe_pin((unsigned char)RESET_BUTTON);
}
int Dionysus::program_fpga(){
  return this->strobe_pin((unsigned char)PROGRAM_BUTTON);
}

