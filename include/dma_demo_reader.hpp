#ifndef __DMA_DEMO_READER_HPP__
#define __DMA_DEMO_READER_HPP__

#define DMA_DEMO_READER_DEVICE_ID 100
#define DMA_DEMO_READER_DEVICE_SUB_ID 0

#define BLOCKING true

#include "driver.hpp"

static uint32_t get_dma_reader_device_type(){
  return (uint32_t) DMA_DEMO_READER_DEVICE_ID;
}


class DMA_DEMO_READER : public Driver {
  private:
    const static uint32_t DMA_BASE0                   = 0x00000000;
    const static uint32_t DMA_BASE1                   = 0x00010000;

    const static uint32_t DMA_SIZE                    = 0x00010000;


    bool debug;
    DMA *dma;
  public:
    DMA_DEMO_READER(Nysa *nysa, uint32_t dev_addr, bool debug = false);
    ~DMA_DEMO_READER();

    //Control
    void enable_dma_reader(bool enable);
    void reset_dma_reader();
    uint32_t get_buffer_size();

    //Data transfer
    void dma_read(uint8_t *buffer);
};


#endif //__DMA_DEMO_READER_HPP__
