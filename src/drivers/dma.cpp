#include "dma.hpp"



DMA::DMA (Nysa *nysa, uint32_t dev_addr, bool debug){
  this->nysa = nysa;
  this->dev_addr = dev_addr;
  this->debug = debug;

  this->SIZE = 0;
  this->BASE[0] = 0;
  this->BASE[1] = 0;
  this->REG_BASE[0] = REG_UNINITIALIZED;
  this->REG_SIZE[0] = REG_UNINITIALIZED;
  this->REG_BASE[1] = REG_UNINITIALIZED;
  this->REG_SIZE[1] = REG_UNINITIALIZED;
  this->continuous_read = true;
  this->immediate_read = true;
  this->blocking = true;
  this->timeout = 1000;
}
DMA::~DMA(){
}

//DMA Setup
int DMA::setup(uint32_t base0,
          uint32_t base1,
          uint32_t size,
          uint32_t reg_base0,
          uint32_t reg_size0,
          uint32_t reg_base1,
          uint32_t reg_size1,
          bool     continuous_read,
          bool     immediate_read,
          bool     blocking){

  this->SIZE = size;
  this->BASE[0] = base0;
  this->BASE[1] = base1;
  this->REG_BASE[0] = reg_base0;
  this->REG_SIZE[0] = reg_size0;
  this->REG_BASE[1] = reg_base1;
  this->REG_SIZE[1] = reg_size1;
  this->continuous_read = continuous_read;
  this->immediate_read = immediate_read;
  this->blocking = blocking;

  //Setup the core
  this->nysa->write_register(this->dev_addr, REG_BASE[0], this->BASE[0]);
  this->nysa->write_register(this->dev_addr, REG_BASE[1], this->BASE[1]);
  return 0;
};

int DMA::set_base(int index, uint32_t base_address){
  if (index < 0){
    return -1;
  }
  if (index > 1){
    return -1;
  }
  this->BASE[index] = base_address;
  if (REG_BASE[index] == REG_UNINITIALIZED){
    return -2;
  }
  this->nysa->write_register(this->dev_addr, REG_BASE[index], this->BASE[index]);
}

void DMA::set_size(uint32_t size){
  this->SIZE = size;
}

void DMA::set_base_register(int index, uint32_t reg_base){
  this->REG_BASE[index] = reg_base;
}

void DMA::set_size_register(int index, uint32_t reg_size){
  this->REG_SIZE[index] = reg_size;
}

void DMA::set_timeout(uint32_t timeout){
  this->timeout = timeout;
}

void DMA::enable_continuous_read(bool enable){
  this->continuous_read = enable;
}

void DMA::enable_immediate_read(bool enable){
  this->immediate_read = enable;
}

void DMA::enable_blocking(bool enable){
  this->blocking = enable;
}

//Write
int write(uint8_t *buffer, uint32_t size){
  return 0xFF;
}
//Read
int read(uint8_t *buffer, uint32_t size){
  return 0xFF;
}

