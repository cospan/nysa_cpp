#ifndef __NYSA_H__
#define __NYSA_H__

#include "dionysus.hpp"

#define CHECK_NYSA_ERROR(x)                       \
do{                                               \
   if (retval < 0){                               \
     if (this->debug) printf (x " %d\n", retval); \
     return retval;                               \
   }                                              \
}while(0)



class Nysa {
  private:
    bool debug;

    int parse_drt();

  public:
    Nysa (bool debug = false);
    ~Nysa();

    int open();
    int close();

    //Low Level interface
    int write_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t data);
    int read_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t *data);

    int write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);
    int read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);

    int write_memory(uint32_t address, uint8_t *buffer, uint32_t size);
    int read_memory(uint32_t address, uint8_t *buffer, uint32_t size);

    int wait_for_interrupts(uint32_t timeout);

    //Helper Functions
    int set_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit);
    int clear_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit);

    //DRT
    int pretty_print_drt();
    int read_drt();

    int get_drt_version(uint32_t version);

    int get_drt_device_count(int *count);
    int get_drt_device_type(uint32_t index, uint32_t *type);
    int is_memory_device(uint32_t index, bool *memory_bus);
    int get_drt_device_size(uint32_t index, uint32_t *size);
    int get_drt_device_flags(uint32_t index, uint32_t *flags);

    int ping();

    int pretty_print_crash_report();
    int get_crash_report(uint32_t *buffer);
};
#endif //__NYSA_H__
