#ifndef __NYSA_DRIVER_H__
#define __NYSA_DRIVER_H__


/*
 * DMA Controller
 *
 * For a reference on how this inner working read 'nysa/docs/dma.txt'
 *
 */

#include "nysa.hpp"
#include "driver.hpp"

#define REG_UNINITIALIZED 0xFFFFFFFF

typedef enum _BLOCK_STATE BLOCK_STATE;

enum rxtx_strategy {
  IMMEDIATE     = 0,
  CADENCE       = 1,
  SINGLE_BUFFER = 2
};

class DMA {
  private:

    Nysa                *nysa;
    Driver              *driver;
    bool                debug;
    uint32_t            dev_addr;
    uint32_t            SIZE;
    uint32_t            REG_STATUS;
    uint32_t            BASE[2];
    uint32_t            REG_BASE[2];
    uint32_t            REG_SIZE[2];
    bool                blocking;
    uint32_t            timeout;
                        
    uint8_t             status_bit_finished[2];
    uint8_t             status_bit_empty[2];

    enum rxtx_strategy  strategy;
    bool                block_select;
    BLOCK_STATE         block_state[2];

    void                update_block_state(uint32_t interrupts);

  public:
  DMA (Nysa *nysa, Driver *driver, uint32_t dev_addr, bool debug = false);
  ~DMA();

  //DMA Setup
  int setup(uint32_t reg_status,
            uint32_t base0,
            uint32_t base1,
            uint32_t size,
            uint32_t reg_base0,
            uint32_t reg_size0,
            uint32_t reg_base1,
            uint32_t reg_size1,
            bool     blocking = true,
            enum rxtx_strategy = CADENCE);

  void set_status_bits( uint8_t finished0,
                        uint8_t finished1,
                        uint8_t empty0,
                        uint8_t empty1,
  void set_timeout(uint32_t timeout);
  void enable_blocking(bool enable);
  void set_strategy(enum rxtx_strategy = CADENCE);

  //Write
  int write(uint8_t *buffer, uint32_t size);
  //Read
  int read(uint8_t *buffer, uint32_t size);
};

#endif //__NYSA_DRIVER_H__
