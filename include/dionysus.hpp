#ifndef __DIONYSUS_HPP__
#define __DIONYSUS_HPP__

#include <libusb.h>
#include <queue>
#include "ftdi.h"
#include "nysa.hpp"
#include <cstring>

#define DIONYSUS_VID 0x0403
#define DIONYSUS_PID 0x8530

/*
const DIONYSUS_ERROR[] = {
  "error"
};
*/


typedef struct _state_t state_t;
typedef struct _command_header_t command_header_t;
typedef struct _response_header_t response_header_t;

class Dionysus : public Nysa {

  private:
    bool usb_is_open;
    bool debug;
    bool comm_mode;
    uint16_t vendor;
    uint16_t product;
    //used to keep track of buffers
    std::queue<uint8_t *> buffer_queue;
    std::queue<uint8_t *> buffers;

    //used to keep track of empty transfers
    std::queue<struct libusb_transfer *> transfer_queue;
    //Used to keep track of transfers in progress
    std::queue<struct libusb_transfer *> transfers;
    state_t * state;

    struct ftdi_context * ftdi;

    //Functions
    int strobe_pin(unsigned char pin);
    int set_control_mode();
    int set_comm_mode();

    void usb_constructor();
    void usb_destructor();
    int usb_open(int vendor, int product);

   public:
    //Constructor, Destructor
    Dionysus(bool debug = false);
    ~Dionysus();

    //Implementations of Nysa classes
    int write_memory(uint32_t address, uint8_t *buffer, uint32_t size);
    int read_memory(uint32_t address, uint8_t *buffer, uint32_t size);


    int write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);
    int read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size);

    int wait_for_interrupts(uint32_t timeout, uint32_t *interrupts);

    int ping();

    int crash_report(uint32_t *buffer);

    //Properties
    int open(int vendor = DIONYSUS_VID, int product = DIONYSUS_PID);
    int close();
    bool is_open();
    int reset();

    void cancel_all_transfers();

    /* I/O */
    int read(uint32_t header_len, uint8_t *buffer, uint32_t size, uint32_t timeout = 1000);
    int write(uint32_t header_len, unsigned char *buf, int size, uint32_t timeout = 1000);
    int read_sync(uint8_t *buffer, uint16_t size);
    int write_sync(uint8_t *buffer, uint16_t size);


    //Bit Control
    int soft_reset();
    int program_fpga();
    int print_status(bool get_status, uint16_t in_status);
};

#endif //__DIONYSUS_HPP__
