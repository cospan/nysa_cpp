#include "dionysus_local.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <ftdi.h>
#include <cstdlib>
#include <libusb.h>

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
  if (this->debug) printf ("Button Mask: 0x%02X\n", (unsigned char) BUTTON_BITMASK);
  retval = ftdi_set_bitmode(bb_ftdi, (unsigned char) BUTTON_BITMASK, BITMODE_BITBANG);
    CHECK_ERROR("Failed to set bitmode");

  retval = ftdi_read_pins(bb_ftdi, &pins);
    CHECK_ERROR("Failed to read pins");
  //pins = ~pins;
  pin_command[0] = (pins & ~pin);
  //pin_command[0] = (pins | 0xFF);
  if (this->debug) {
    printf ("Write Buttons Values: 0x%02X\n", pin_command[0]);
  }

  retval = ftdi_write_data(bb_ftdi, pin_command, 1);
    CHECK_ERROR("Failed to write data to BB Mode pins");
  //Sleep for 300 ms
  usleep(300000);
  pin_command[0] = (pins | pin);
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

