#ifndef __DIONYSUS_HPP__
#define __DIONYSUS_HPP__

#include <libusb.h>
#include <queue>
#include "ftdi.hpp"

#define DIONYSUS_VID 0x0403
#define DIONYSUS_PID 0x8530

#define RESET_BUTTON 0x20
#define PROGRAM_BUTTON 0x10
#define BUTTON_BITMASK (RESET_BUTTON | PROGRAM_BUTTON)

#define NUM_TRANSFERS 256
#define FTDI_BUFFER_SIZE 4096
#define DEFAULT_BUFFER_SIZE (NUM_TRANSFERS * FTDI_BUFFER_SIZE)
//#define PACKETS_PER_TRANSFER (FTID_BUFFER_SIZE / 512)


//Error Conditions
#define CHECK_ERROR(x) do{                                              \
                          if (retval < 0){                              \
                            if (this->debug) printf (x " %d\n", retval);\
                            return retval;                              \
                          }                                             \
                       }while(0)

/*
const DIONYSUS_ERROR[] = {
  "error"
};
*/

typedef struct _state_t state_t;

class Dionysus : protected Ftdi::Context {
  private:
    bool debug;
    bool comm_mode;
    uint16_t vendor;
    uint16_t product;
    //used to keep track of empty transfers
    std::queue<struct libusb_transfer *> transfer_queue;
    //Used to keep track of transfers in progress
    std::queue<struct libusb_transfer *> transfers;
    state_t * state;

    Ftdi::Context bitbang_context;

    //Functions
    int set_control_mode();
    void sleep(uint32_t useconds = 200000);

    int strobe_pin(unsigned char pin);

  public:
    //Constructor, Destructor
    Dionysus(bool debug = false);
    ~Dionysus();

    //Properties
    int open(int vendor = DIONYSUS_VID, int product = DIONYSUS_PID);
    int close();
    bool is_open();
    int reset();

    void cancel_all_transfers();

    /* I/O */
    int read(uint8_t *buf, uint32_t size);
    int write(unsigned char *buf, int size);

    //Bit Control
    int soft_reset();
    int program_fpga();
};

#endif //__DIONYSUS_HPP__
