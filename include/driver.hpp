#ifndef __NYSA_DRIVER__H__
#define __NYSA_DRIVER__H__

#include "nysa.hpp"
#include <stdint.h>

#define REG_UNINITIALIZED 0xFFFFFFFF
/*
 * Base Class for all Nysa based drivers
 *
 * Nysa: a reference to the Nysa controller, this may be subclassed by an
 * Implementation such as Dionysus
 *
 * dev_index: This is the reference to the index
 * dev_id: Identification for the device, this should be set in the
 *  constructor
 * dev_sub_id: Sub identificaiton of the device, this should be set
 *  in the constructor
 * dev_unique_id: unique ID when there are multiple devices of the same
 *  type in the image
 *    (When set to 0 the find funtion will ignore this value)
 */



static char * get_message(int retval);

class DMA;

class Driver {

  //DMA is only used with a driver and need access to protected functions
  //but DMA may not be used in all drivers and multiple DMAs may be used
  friend class DMA;

  private:
    Nysa * n;
    uint16_t dev_index;
    uint16_t id;
    uint16_t sub_id;
    uint16_t  unique_id;
    int message_id;
    bool debug;
    int error;

  protected:
    /*Nysa Functions
     * These won't be visible to the user but this is what you use to talk to
     * a device
     */
    void write_periph_data(uint32_t addr, uint8_t *buffer, uint32_t size);
    void read_periph_data(uint32_t addr, uint8_t *buffer, uint32_t size);

    void write_register(uint32_t reg_addr, uint32_t data);
    uint32_t read_register(uint32_t reg_addr);

    void set_register_bit(uint32_t reg_addr, uint8_t bit);
    void clear_register_bit(uint32_t reg_addr, uint8_t bit);
    bool read_register_bit(uint32_t reg_addr, uint8_t bit);

    void set_device_id(uint16_t id);
    void set_device_sub_id(uint16_t sub_id);


  public:
    Driver(Nysa *nysa, bool debug = false);
    ~Driver();

    int find_device();          //Find the device set up in the constructor
    void set_unique_id(uint16_t id);
    bool is_interrupt_for_device(uint32_t interrupts);

    virtual int open();         //Initialized a device that is found inside the DRT
    virtual int close();        //Clean up the device
};

/*
 * DMA Controller
 *
 * For a reference on how this inner working read 'nysa/docs/dma.txt'
 *
 */

enum _RXTX_STRATEGY {
  IMMEDIATE     = 0,
  CADENCE       = 1,
  SINGLE_BUFFER = 2
};

typedef enum _RXTX_STRATEGY RXTX_STRATEGY;

class DMA {
  private:

    Nysa                *nysa;
    Driver              *driver;
    bool                debug;
    bool                writing;
    uint32_t            dev_addr;
    uint32_t            SIZE;
    uint32_t            REG_STATUS;
    uint32_t            BASE[2];
    uint32_t            REG_BASE[2];
    uint32_t            REG_SIZE[2];
    bool                blocking;
    uint32_t            timeout;

    uint32_t            status_bit_finished[2];
    uint32_t            status_bit_empty[2];

    RXTX_STRATEGY       strategy;
    bool                block_select;
    uint32_t            block_state[2];
    uint32_t            read_state;
    bool                test_bit;


    void update_block_state(uint32_t interrupts);
    void process_status(uint32_t status);
    int  setup(
            uint32_t mem_base0,
            uint32_t mem_base1,
            uint32_t size,
            uint32_t reg_status,
            uint32_t reg_base0,
            uint32_t reg_size0,
            uint32_t reg_base1,
            uint32_t reg_size1,
            bool     blocking = true,
            RXTX_STRATEGY strategy = CADENCE);
  public:
  DMA (Nysa *nysa, Driver *driver, uint32_t dev_addr, bool debug = false);
  ~DMA();

  //DMA Setup
  int setup_write(
            uint32_t mem_base0,
            uint32_t mem_base1,
            uint32_t size,
            uint32_t reg_status,
            uint32_t reg_base0,
            uint32_t reg_size0,
            uint32_t reg_base1,
            uint32_t reg_size1,
            bool     blocking = true,
            RXTX_STRATEGY strategy = CADENCE);

  int setup_read(
            uint32_t mem_base0,
            uint32_t mem_base1,
            uint32_t size,
            uint32_t reg_status,
            uint32_t reg_base0,
            uint32_t reg_size0,
            uint32_t reg_base1,
            uint32_t reg_size1,
            bool     blocking = true,
            RXTX_STRATEGY strategy = CADENCE);



  void set_status_bits( uint8_t finished0,
                        uint8_t finished1,
                        uint8_t empty0,
                        uint8_t empty1);
  void set_timeout(uint32_t timeout);
  void enable_blocking(bool enable);
  void set_strategy(RXTX_STRATEGY = CADENCE);

  //Write
  int write(uint8_t *buffer);
  //Read
  int read(uint8_t *buffer);
};



#endif
