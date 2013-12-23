#ifndef __NYSA_H__
#define __NYSA_H__

#include <stdint.h>
#include <stdlib.h>
#include "print_colors.hpp"

#define printd(x)                                 \
do{                                               \
  if (this->debug) printf (P_GREEN);              \
  if (this->debug) printf ("%s(): ", __func__);   \
  if (this->debug) printf (x);                    \
  if (this->debug) printf (P_NORMAL);             \
}while(0)

#define CHECK_NYSA_ERROR(x)                       \
do{                                               \
   if (retval < 0){                               \
     if (this->debug) printf (x " %d\n", retval); \
     return retval;                               \
   }                                              \
}while(0)

class Nysa {
  private:
    uint8_t * drt;
    bool debug;
    int parse_drt();

    //DRT Values
    int num_devices;
    uint8_t version;

  public:
    Nysa (bool debug = false);
    ~Nysa();

    int open();
    int close();

    //Low Level interface
    virtual int write_memory(uint32_t address, uint8_t *buffer, uint32_t size);
    virtual int read_memory(uint32_t address, uint8_t *buffer, uint32_t size);

    virtual int write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);
    virtual int read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);

    virtual int wait_for_interrupts(uint32_t timeout, uint32_t *interrupts);

    virtual int ping();

    virtual int crash_report(uint32_t *buffer);

    //Helper Functions
    int write_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t data);
    int set_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit);
    int clear_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit);

    int read_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t *data);
    int read_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit, bool *value);

    //DRT
    int pretty_print_drt();
    int read_drt();

    int get_drt_version();

    int get_drt_device_count();
    uint16_t get_drt_device_type(uint32_t index);
    uint16_t get_drt_device_sub_type(uint32_t index);
    uint16_t get_drt_device_user_id(uint32_t index);
    uint32_t get_image_id();
    uint32_t get_board_id();
    bool is_memory_device(uint32_t index);
    uint32_t get_drt_device_size(uint32_t index);

    int get_drt_device_flags(uint32_t index, uint16_t *nysa_flags, uint16_t *dev_flags);
    uint32_t get_drt_device_addr(uint32_t index);

    int pretty_print_crash_report();

    uint32_t find_device(uint32_t device_type, uint32_t subtype = 0, uint32_t id = 0);
};
#endif //__NYSA_H__
