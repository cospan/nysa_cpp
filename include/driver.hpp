#ifndef __NYSA_DRIVER__H__
#define __NYSA_DRIVER__H__

#include "nysa.hpp"
#include <stdint.h>

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


class Driver {

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


#endif
