#ifndef __NYSA_DRIVER_H__
#define __NYSA_DRIVER_H__


/*
 * DMA Controller
 *
 * For a reference on how this inner working read 'nysa/docs/dma.txt'
 *
 */

#include "nysa.hpp"

#define REG_UNINITIALIZED 0xFFFFFFFF

class DMA {
  private:
    Nysa      *nysa;
    bool      debug;
    uint32_t  dev_addr;
    uint32_t  SIZE;
    uint32_t  BASE[2];
    uint32_t  REG_BASE[2];
    uint32_t  REG_SIZE[2];
    bool      continuous_read;
    bool      immediate_read;
    bool      blocking;
    uint32_t  timeout;

  public:
  DMA (Nysa *nysa, uint32_t dev_addr, bool debug = false);
  ~DMA();

  //DMA Setup
  int setup(uint32_t base0,
            uint32_t base1,
            uint32_t size,
            uint32_t reg_base0,
            uint32_t reg_size0,
            uint32_t reg_base1,
            uint32_t reg_size1,
            bool     continuous_read = true,
            bool     immediate_read = true,
            bool     blocking = true);

  int set_base(int index, uint32_t base_address);
  void set_size(uint32_t size);
  void set_base_register(int index, uint32_t reg_base);
  void set_size_register(int index, uint32_t reg_size);
  void set_timeout(uint32_t timeout);
  void enable_continuous_read(bool enable);
  void enable_immediate_read(bool enable);
  void enable_blocking(bool enable);

  //Write
  int write(uint8_t *buffer, uint32_t size);
  //Read
  int read(uint8_t *buffer, uint32_t size);
};

#endif //__NYSA_DRIVER_H__
