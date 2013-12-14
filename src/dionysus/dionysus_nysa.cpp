#include "dionysus_local.hpp"
#include <stdio.h>

//Nysa Interface for Dionysus

static uint32_t populate_ping_command (command_header_t *ch){
  ch->id = ID;
  ch->command = PING;
  ch->data_count[0] = 0x00;
  ch->data_count[1] = 0x00;
  ch->data_count[2] = 0x00;

  ch->address.dev_addr = 0x00;
  ch->address.reg_addr[0] = 0x00;
  ch->address.reg_addr[1] = 0x00;
  ch->address.reg_addr[2] = 0x00;

  ch->data.data = 0x00;
  return COMMAND_HEADER_LEN;
}
static uint32_t populate_write_periph_command(command_header_t * ch, uint32_t dword_len, uint8_t dev_addr, uint32_t reg_address){
  ch->id = ID;
  ch->command = WRITE;
  ch->data_count[0] = (dword_len >> 16) & 0xFF;
  ch->data_count[1] = (dword_len >> 8 ) & 0xFF;
  ch->data_count[2] = (dword_len      ) & 0xFF;

  ch->address.dev_addr = dev_addr;
  ch->address.reg_addr[0] = (reg_address >> 16) & 0xFF;
  ch->address.reg_addr[1] = (reg_address >> 8 ) & 0xFF;
  ch->address.reg_addr[2] = (reg_address      ) & 0xFF;
  ch->data.data = 0x00;
  return COMMAND_HEADER_LEN;
}
static uint32_t populate_write_mem_command(command_header_t* ch, uint32_t dword_len, uint32_t address){
  ch->id = ID;
  ch->command = MEM_FLAG | WRITE;
  ch->data_count[0] = (dword_len >> 16) & 0xFF;
  ch->data_count[1] = (dword_len >> 8 ) & 0xFF;
  ch->data_count[2] = (dword_len      ) & 0xFF;

  ch->address.mem_addr[0] = (address >> 24) & 0xFF;
  ch->address.reg_addr[1] = (address >> 16) & 0xFF;
  ch->address.reg_addr[2] = (address >> 8 ) & 0xFF;
  ch->address.reg_addr[3] = (address      ) & 0xFF;
  ch->data.data = 0x00;
  return COMMAND_HEADER_LEN;
}
static uint32_t populate_read_periph_command(command_header_t * ch, uint32_t dword_len, uint8_t dev_addr, uint32_t reg_address){
  ch->id = ID;
  ch->command = READ;
  ch->data_count[0] = (dword_len >> 16) & 0xFF;
  ch->data_count[1] = (dword_len >> 8 ) & 0xFF;
  ch->data_count[2] = (dword_len      ) & 0xFF;

  ch->address.dev_addr = dev_addr;
  ch->address.reg_addr[0] = (reg_address >> 16) & 0xFF;
  ch->address.reg_addr[1] = (reg_address >> 8 ) & 0xFF;
  ch->address.reg_addr[2] = (reg_address      ) & 0xFF;
  ch->data.data = 0x00;

  return COMMAND_HEADER_LEN;
}
static uint32_t populate_read_mem_command(command_header_t * ch, uint32_t dword_len, uint32_t address){
  ch->id = ID;
  ch->command = MEM_FLAG | READ;

  ch->data_count[0] = (dword_len >> 16) & 0xFF;
  ch->data_count[1] = (dword_len >> 8 ) & 0xFF;
  ch->data_count[2] = (dword_len      ) & 0xFF;

  ch->address.mem_addr[0] = (address >> 24) & 0xFF;
  ch->address.reg_addr[1] = (address >> 16) & 0xFF;
  ch->address.reg_addr[2] = (address >> 8 ) & 0xFF;
  ch->address.reg_addr[3] = (address      ) & 0xFF;
  ch->data.data = 0x00;
  return COMMAND_HEADER_LEN;
}
static uint32_t populate_interrupt_command (command_header_t *ch){
  ch->id = ID;
  ch->command = INTERRUPT;
  ch->data_count[0] = 0x00;
  ch->data_count[1] = 0x00;
  ch->data_count[2] = 0x00;

  ch->address.dev_addr = 0x00;
  ch->address.reg_addr[0] = 0x00;
  ch->address.reg_addr[1] = 0x00;
  ch->address.reg_addr[2] = 0x00;
  ch->data.data = 0x00;
  return COMMAND_HEADER_LEN;
}


//Nysa Overrides
int Dionysus::write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  //Construct a packet header
  int retval = 0;
  uint32_t header_len = populate_write_periph_command(&this->state->command_header, (size / 4), dev_addr, addr);
  retval = this->write(header_len, buffer, size);
    CHECK_ERROR("Failed to Write Data");
  retval = this->read(header_len, NULL, 0);
    CHECK_ERROR("Failed to Read Data");
  return 0;
}

int Dionysus::read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  //Construct a packet header
  int retval = 0;
  uint32_t header_len = populate_read_periph_command(&this->state->command_header, (size / 4), dev_addr, addr);
  //retval = this->write_sync((uint8_t *) &this->state->command_header, header_len);
  retval = this->write(header_len, NULL, 0);
  //retval = this->write(header_len, buffer, size);
    CHECK_ERROR("Failed to Write Data");
  retval = this->read(RESPONSE_HEADER_LEN, buffer, size);
    CHECK_ERROR("Failed to Read Data");
  return 0;

}

int Dionysus::write_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  //Construct a packet header
  int retval = 0;
  uint32_t header_len = populate_write_mem_command(&this->state->command_header, (size / 4), address);
  retval = this->write(header_len, buffer, size);
    CHECK_ERROR("Failed to Write Data");
  retval = this->read(RESPONSE_HEADER_LEN, NULL, 0);
    CHECK_ERROR("Failed to Read Data");
  return 0;
}
int Dionysus::read_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  //Construct a packet header
  int retval = 0;
  uint32_t header_len = populate_read_mem_command(&this->state->command_header, (size / 4), address);
  //retval = this->write_sync((uint8_t *) &this->state->command_header, header_len);
  retval = this->write(header_len, NULL, 0);
    CHECK_ERROR("Failed to Write Data");
  retval = this->read(RESPONSE_HEADER_LEN, buffer, size);
    CHECK_ERROR("Failed to Read Data");
  return 0;
}

int Dionysus::wait_for_interrupts(uint32_t timeout, uint32_t *interrupts){
  //Construct a packet header
  int retval = 0;
  uint32_t header_len = populate_interrupt_command(&this->state->command_header);
  uint8_t buffer[RESPONSE_INT_HEADER_LEN];
  uint8_t local_interrupts[4];
  *interrupts = 0;
  retval = this->read(RESPONSE_INT_HEADER_LEN, (uint8_t *) &local_interrupts, 4);
    CHECK_ERROR("Failed to Read Data");

  *interrupts = local_interrupts[0] << 24 | \
                local_interrupts[1] << 16 | \
                local_interrupts[2] << 8  | \
                local_interrupts[3];
  return 0;
}

int Dionysus::ping(){
  if (this->debug) printf ("Ping...\n");
  int retval = 0;
  uint32_t len  = populate_ping_command(&this->state->command_header);
  //printf ("Length of write transfer: %d\n", len);
  retval = this->write(len, NULL, 0);
  //retval = Dionysus::write_sync((uint8_t *)&this->state->command_header, len);
  //  CHECK_ERROR("Failed to Write Data");
  //retval = this->read(RESPONSE_HEADER_LEN, NULL, 0);
  //retval = Dionysus::read_sync((uint8_t *)&this->state->response_header, RESPONSE_HEADER_LEN);
  //  CHECK_ERROR("Failed to read a Ping Response");

  retval = Dionysus::read(PING_RESPONSE_HEADER_LEN, NULL, 0);
    CHECK_ERROR("Failed to read a Ping Response");

  if (this->state->response_header.id != ID_RESPONSE){
    printd("Response from a Ping is incorrect\n");
    return -1;
  }
  if (this->state->response_header.status != ((~PING) & 0xFF)){
    if (this->debug){
      printf("Ping Status is incorrect, should be: 0x%02X, instead read: 0x%02X\n",
          ~PING,
          this->state->response_header.status);
    }
  }
  //this->print_status(true, 0);
  return 0;
}

int Dionysus::crash_report(uint32_t *buffer){
  return -1;
}
