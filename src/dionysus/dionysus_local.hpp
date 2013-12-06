#ifndef __DIONYSUS_LOCAL_H__
#define __DIONYSUS_LOCAL_H__
#include "dionysus.hpp"

#define RESET_BUTTON 0x40
#define PROGRAM_BUTTON 0x10
#define BUTTON_BITMASK (RESET_BUTTON | PROGRAM_BUTTON)

#define NUM_TRANSFERS 256
//#define FTDI_BUFFER_SIZE 4096
#define FTDI_BUFFER_SIZE 4096
#define DEFAULT_BUFFER_SIZE (NUM_TRANSFERS * FTDI_BUFFER_SIZE)

//#define BUFFER_SIZE 510
//#define BUFFER_SIZE 65536
#define BUFFER_SIZE 512
//#define BUFFER_SIZE DEFAULT_BUFFER_SIZE

#define ID    0xCD
#define PING  0x00
#define WRITE 0x01
#define READ  0x02
#define CRASH 0x03
#define INTERRUPT 0x0F

#define MEM_FLAG 0x10

#define COMMAND_HEADER_LEN 9

#define ID_RESPONSE 0xDC
#define PING_RESPONSE_HEADER_LEN 5
#define RESPONSE_HEADER_LEN 9
#define RESPONSE_INT_HEADER_LEN 13


#define  MODEM_CTS  (1 << 4)  /* Clear to send                */
#define  MODEM_DSR  (1 << 5)  /* Data set ready               */
#define  MODEM_RI   (1 << 6)  /* Ring indicator               */
#define  MODEM_RLSD (1 << 7)  /* Carrier detect               */
#define  MODEM_DR   (1 << 8)  /* Data ready                   */
#define  MODEM_OE   (1 << 9)  /* Overrun error                */
#define  MODEM_PE   (1 << 10) /* Parity error                 */
#define  MODEM_FE   (1 << 11) /* Framing error                */
#define  MODEM_BI   (1 << 12) /* Break interrupt              */
#define  MODEM_THRE (1 << 13) /* Transmitter holding register */
#define  MODEM_TEMT (1 << 14) /* Transmitter empty            */
#define  MODEM_RCVE (1 << 15) /* Error in RCVR FIFO           */



//Error Conditions
#define CHECK_ERROR(x) do{                                              \
                          if (retval < 0){                              \
                            if (this->debug) printf ("%s(): ", __func__); \
                            if (this->debug) printf (x " %d\n", retval);\
                            return retval;                              \
                          }                                             \
                       }while(0)


#define printds(x)    do{                                               \
                        if (state->debug) printf ("%s(): ", __func__);  \
                        if (state->debug) printf (x);                   \
                       }while(0)

struct _command_header_t {
  uint8_t id;
  uint8_t command;
  uint8_t data_count[3];
  union {
    struct {
      uint8_t dev_addr;
      uint8_t reg_addr[3];
    };
    uint8_t mem_addr[4];
  } address;
  union {
    uint32_t data;
    uint8_t byte_data[4];
  }data;
};

struct _response_header_t {
//  uint16_t modem_status;
  uint8_t id;
  uint8_t status;
  uint8_t data_count[3];
  union {
    struct {
      uint8_t dev_addr;
      uint8_t reg_addr[3];
    };
    uint8_t mem_addr[4];
  } address;
  union {
    uint32_t data;
    uint8_t byte_data[4];
  }data;
};


struct _state_t {

  std::queue<struct libusb_transfer *> *transfer_queue;
  std::queue<uint8_t *> *buffer_queue;
  //Context of FTDI to continue transactions
  struct libusb_context * usb_ctx;
  struct libusb_device_handle * usb_dev;
  int    in_ep;
  int    out_ep;

  uint32_t transfer_index;

  //Dionysus Memory Buffer used to store data
  uint8_t * buffer;
  uint32_t buffer_size;
  uint32_t buffer_pos;

  uint32_t header_pos;
  uint32_t header_size;

  uint32_t usb_total_size;
  int32_t  usb_size_left;
  uint32_t usb_pos;
  //USB Total size position
  uint32_t usb_actual_pos;


  uint32_t read_data_count;
  uint32_t read_dev_addr;
  uint32_t read_reg_addr;
  uint32_t read_mem_addr;
  bool mem_response;

  //Reference to the class
  Dionysus *d;
  bool header_found;
  command_header_t command_header;
  response_header_t response_header;

  int finished;
  bool debug;
  int error;
};



#endif
