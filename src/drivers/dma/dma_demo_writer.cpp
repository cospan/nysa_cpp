#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include "dma_demo_writer.hpp"

enum DMA_DEMO_WRITER_REGISTERS{
  REG_CONTROL             = 0,
  REG_STATUS              = 1,
  REG_MEM_0_BASE          = 2,
  REG_MEM_0_SIZE          = 3,
  REG_MEM_1_BASE          = 4,
  REG_MEM_1_SIZE          = 5,
  REG_WRITTEN_SIZE        = 6

};

enum CONTROL_REG {
  ENABLE              = 0,
  ENABLE_INTERRUPTS   = 1,

  RESET               = 4
};

enum STATUS_REG {
  STATUS_0_EMPTY      = 0,
  STATUS_1_EMPTY      = 1,
  STATUS_0_FINISHED   = 2,
  STATUS_1_FINISHED   = 3
};

DMA_DEMO_WRITER::DMA_DEMO_WRITER(Nysa *nysa, uint32_t dev_addr, bool debug) : Driver(nysa, debug){
  this->debug = debug;
  this->set_device_id(DMA_DEMO_WRITER_DEVICE_ID);
  this->set_device_sub_id(DMA_DEMO_WRITER_DEVICE_SUB_ID);
  this->find_device();
  this->dma = new DMA(nysa, this, dev_addr, debug);
  this->dma->setup_write( DMA_BASE0,
                          DMA_BASE1,
                          DMA_SIZE,
                          REG_STATUS,
                          REG_MEM_0_BASE,
                          REG_MEM_0_SIZE,
                          REG_MEM_1_BASE,
                          REG_MEM_1_SIZE,
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
void DMA_DEMO_WRITER::enable_dma_writer(bool enable){
  if (enable) {
    this->set_register_bit(REG_CONTROL, ENABLE);
    this->set_register_bit(REG_CONTROL, ENABLE_INTERRUPTS);
  }
  else {
    this->clear_register_bit(REG_CONTROL, ENABLE);
    this->clear_register_bit(REG_CONTROL, ENABLE_INTERRUPTS);
  }
  printf ("%s(): Control Register: 0x%08X\n", __func__, this->read_register(REG_CONTROL));
}
void DMA_DEMO_WRITER::reset_dma_writer(){
  //printf ("%s(): Reset...\n", __func__);
  this->set_register_bit(REG_CONTROL, RESET);
  usleep(30000);
  //printf ("%s(): Reset finished...\n", __func__);
  this->clear_register_bit(REG_CONTROL, RESET);
}
bool DMA_DEMO_WRITER::finished(){
  uint32_t status;
  status = this->read_register(REG_STATUS);
  if (status &  ((1 << STATUS_0_FINISHED) | (1 << STATUS_1_FINISHED))) {
    return true;
  }
  return false;
}
bool DMA_DEMO_WRITER::empty(){
  uint32_t status;
  status = this->read_register(REG_STATUS);
  if (status &  ((1 << STATUS_0_EMPTY) | (1 << STATUS_1_EMPTY))) {
    return true;
  }
  return false;
}
uint32_t DMA_DEMO_WRITER::get_buffer_size(){
  return DMA_SIZE;
}
uint32_t DMA_DEMO_WRITER::get_written_size(){
  return this->read_register(REG_WRITTEN_SIZE);
}
//Data transfer
void DMA_DEMO_WRITER::dma_write(uint8_t *buffer){
  printf ("%s(): Writing...\n", __func__);
  this->dma->write(buffer);
  printf ("%s(): Finished Writing\n", __func__);
}

