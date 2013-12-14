#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include "dma_demo_writer.hpp"

enum DMA_DEMO_WRITER_REGISTERS{
  CONTROL             = 0,
  STATUS              = 1,
  MEM_0_BASE          = 2,
  MEM_0_SIZE          = 3,
  MEM_1_BASE          = 4,
  MEM_1_SIZE          = 5
};

enum CONTROL_REG {
  ENABLE              = 0,
  ENABLE_INTERRUPTS   = 1,

  RESET               = 4
};

enum STATUS_REG {
  STATUS_0_FINISHED   = 0,
  STATUS_1_FINISHED   = 1,
  STATUS_0_EMPTY      = 2,
  STATUS_1_EMPTY      = 3
};



DMA_DEMO_WRITER::DMA_DEMO_WRITER(Nysa *nysa, uint32_t dev_addr, bool debug) : Driver(nysa, debug){
  this->debug = debug;
  this->set_device_id(DMA_DEMO_WRITER_DEVICE_ID);
  this->set_device_sub_id(DMA_DEMO_WRITER_DEVICE_SUB_ID);
  this->find_device();
  this->dma = new DMA(nysa, this, dev_addr, debug);
  this->dma->setup(
            STATUS,
            DMA_BASE0,
            DMA_BASE1,
            DMA_SIZE,
            MEM_0_BASE,
            MEM_0_SIZE,
            MEM_1_BASE,
            MEM_1_SIZE,
            BLOCKING,
            CADENCE);

  this->dma->set_status_bits( STATUS_0_FINISHED,
                              STATUS_1_FINISHED,
                              STATUS_0_EMPTY,
                              STATUS_1_EMPTY);
}
DMA_DEMO_WRITER::~DMA_DEMO_WRITER(){
  delete(this->dma);
}
//Control
void DMA_DEMO_WRITER::enable_dma_in(bool enable){
  if (enable) {
    this->set_register_bit(CONTROL, ENABLE);
    this->set_register_bit(CONTROL, ENABLE_INTERRUPTS);
  }
  else {
    this->clear_register_bit(CONTROL, ENABLE);
    this->clear_register_bit(CONTROL, ENABLE_INTERRUPTS);
  }
}
void DMA_DEMO_WRITER::reset_dma_in(){
  this->set_register_bit(CONTROL, RESET);
  usleep(300000);
  this->clear_register_bit(CONTROL, RESET);
}
bool DMA_DEMO_WRITER::finished(){
  uint32_t status;
  status = this->read_register(STATUS);
  if (status &  ((1 << STATUS_0_FINISHED) | (1 << STATUS_1_FINISHED))) {
    return true;
  }
  return false;
}
bool DMA_DEMO_WRITER::empty(){
  uint32_t status;
  status = this->read_register(STATUS);
  if (status &  ((1 << STATUS_0_EMPTY) | (1 << STATUS_1_EMPTY))) {
    return true;
  }
  return false;
}
//Data transfer
void DMA_DEMO_WRITER::dma_write(uint8_t *buffer, uint32_t size){
  this->dma->write(buffer, size);
}
uint8_t * DMA_DEMO_WRITER::dma_read(uint32_t size){
  uint8_t * buffer;
  this->dma->read(buffer, size);
  return buffer;
}

