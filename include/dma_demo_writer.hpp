#ifndef __DMA_DEMO_WRITER_HPP__
#define __DMA_DEMO_WRITER_HPP__

#define DMA_DEMO_WRITER_DEVICE_ID 100
#define DMA_DEMO_WRITER_DEVICE_SUB_ID 0

#define DMA_BASE0 0x00000000
#define DMA_BASE1 0x00010000

#define DMA_SIZE  0x00010000

#define CONTINUOUS_READ true
#define IMMEDIATE_READ true
#define BLOCKING true

#include "driver.hpp"
#include "dma.hpp"

static uint32_t get_dma_in_device_type(){
  return (uint32_t) DMA_DEMO_WRITER_DEVICE_ID;
}

class DMA_DEMO_WRITER : public Driver {

  friend class DMA;

  private:
    bool debug;
    DMA *dma;

  public:
    DMA_DEMO_WRITER(Nysa *nysa, uint32_t dev_addr, bool debug = false);
    ~DMA_DEMO_WRITER();

    //Control
    void enable_dma_in(bool enable);
    void reset_dma_in();
    bool finished();
    bool empty();

    //Data transfer
    void dma_write(uint8_t *buffer, uint32_t size);
    uint8_t * dma_read(uint32_t size);
};


#endif //__DMA_DEMO_WRITER_HPP__
