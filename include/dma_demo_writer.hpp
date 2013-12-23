#ifndef __DMA_DEMO_WRITER_HPP__
#define __DMA_DEMO_WRITER_HPP__

#define DMA_DEMO_WRITER_DEVICE_ID 101
#define DMA_DEMO_WRITER_DEVICE_SUB_ID 0

#define BLOCKING true

#include "driver.hpp"

static uint32_t get_dma_writer_device_type(){
  return (uint32_t) DMA_DEMO_WRITER_DEVICE_ID;
}

class DMA_DEMO_WRITER : public Driver {

  private:
    const static uint32_t DMA_BASE0                   = 0x00000000;
    const static uint32_t DMA_BASE1                   = 0x00010000;

    const static uint32_t DMA_SIZE                    = 0x00010000;
    bool debug;
    DMA *dma;

  public:
    DMA_DEMO_WRITER(Nysa *nysa, uint32_t dev_addr, bool debug = false);
    ~DMA_DEMO_WRITER();

    //Control
    void enable_dma_writer(bool enable);
    void reset_dma_writer();
    bool finished();
    bool empty();
    uint32_t get_buffer_size();

    //Data transfer
    void dma_write(uint8_t *buffer);
};


#endif //__DMA_DEMO_WRITER_HPP__
