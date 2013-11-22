#include "nysa.hpp"
#include <stdio.h>

Nysa::Nysa(bool debug) {
  this->debug = debug;
}

Nysa::~Nysa(){
}

int Nysa::open(){
  return 1;
}
int Nysa::close(){
  return 1;
}

int Nysa::parse_drt(){
  //read the DRT Json File
}

    //Low Level interface
int Nysa::write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  return -1;
}

int Nysa::read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  return -1;
}

int Nysa::write_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  return -1;
}
int Nysa::read_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  return -1;
}

int Nysa::wait_for_interrupts(uint32_t timeout){
  return -1;
}

//Helper Functions
int Nysa::write_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t data){
  //write to only one address in the peripheral address space
  uint8_t d[4];
  d[3] = ((data >> 24) & 0xFF);
  d[2] = ((data >> 16) & 0xFF);
  d[1] = ((data >> 8)  & 0xFF);
  d[0] = ( data        & 0xFF);
  return this->write_periph_data(dev_addr, reg_addr, &d[0], 4);
}

int Nysa::read_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t *data){
  //read from only one address in the peripheral address space
  uint8_t d[4];
  uint32_t retval;
  retval = this->read_periph_data(dev_addr, reg_addr, &d[0], 4);
  CHECK_NYSA_ERROR("Error Reading Peripheral Data");
  *data = (d[3] << 24 | d[2] << 16 | d[1] << 8 | d[0]);
  return 0;
}

int Nysa::set_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit){
  uint32_t reg;
  int retval = 0;

  retval = this->read_register(dev_addr, reg_addr, &reg);
  CHECK_NYSA_ERROR("Error Reading Register");

  reg |= 1 << bit;
  retval = this->write_register(dev_addr, reg_addr, reg);
  CHECK_NYSA_ERROR("Error Writing Register");
  return 0;
}
int Nysa::clear_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit){
  uint32_t reg;
  int retval = 0;
  retval = this->read_register(dev_addr, reg_addr, &reg);
  CHECK_NYSA_ERROR("Error Reading Register");

  reg &= ~(1 << bit);
  retval = this->write_register(dev_addr, reg_addr, reg);
  CHECK_NYSA_ERROR("Error Writing Register");
  return 0;
}

//DRT
int Nysa::pretty_print_drt(){
  return -1;
}
int Nysa::read_drt(){
  return -1;
}

int Nysa::get_drt_version(uint32_t version){
  return -1;
}

int Nysa::get_drt_device_count(int *count){
  return -1;
}
int Nysa::get_drt_device_type(uint32_t index, uint32_t *type){
  return -1;
}
int Nysa::is_memory_device(uint32_t index, bool *memory_bus){
  return -1;
}
int Nysa::get_drt_device_size(uint32_t index, uint32_t *size){
  return -1;
}
int Nysa::get_drt_device_flags(uint32_t index, uint32_t *flags){
  return -1;
}

int Nysa::ping(){
  return -1;
}

int Nysa::pretty_print_crash_report(){
  return -1;
}
int Nysa::get_crash_report(uint32_t *buffer){
  return -1;
}

