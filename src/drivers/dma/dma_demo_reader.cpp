#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include "dma_demo_reader.hpp"

enum DMA_DEMO_READER_REGISTERS{
  REG_CONTROL             = 0,
  REG_STATUS              = 1,
  REG_MEM_0_BASE          = 2,
  REG_MEM_0_SIZE          = 3,
  REG_MEM_1_BASE          = 4,
  REG_MEM_1_SIZE          = 5
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

DMA_DEMO_READER::DMA_DEMO_READER(Nysa *nysa, uint32_t dev_addr, bool debug) : Driver(nysa, debug){
  this->debug = debug;
  this->set_device_id(DMA_DEMO_READER_DEVICE_ID);
  this->set_device_sub_id(DMA_DEMO_READER_DEVICE_SUB_ID);
  this->find_device();
  this->dma = new DMA(nysa, this, dev_addr, debug);
  this->dma->setup_read ( DMA_BASE0,
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

DMA_DEMO_READER::~DMA_DEMO_READER(){
  delete(this->dma);
}

void DMA_DEMO_READER::enable_dma_reader(bool enable){
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

void DMA_DEMO_READER::reset_dma_reader(){
  this->set_register_bit(REG_CONTROL, RESET);
  usleep(30000);
  this->clear_register_bit(REG_CONTROL, RESET);
}

uint32_t DMA_DEMO_READER::get_buffer_size(){
  return DMA_SIZE;
}

//Data transfer
void DMA_DEMO_READER::dma_read(uint8_t *buffer){
  //printf ("%s(): Reading...\n", __func__);
  this->dma->read(buffer);
  //printf ("%s(): Finished Reading\n", __func__);

}


