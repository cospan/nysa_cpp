#include "nysa.hpp"
#include <stdio.h>

Nysa::Nysa(bool debug) {
  this->debug = debug;
  this->drt = NULL;

  //DRT Settings
  this->num_devices = 0;
  this->version = 0;
}

Nysa::~Nysa(){
  if (this->drt != NULL){
    delete(this->drt);
  }
}

int Nysa::open(){
  return 1;
}
int Nysa::close(){
  return 1;
}

int Nysa::parse_drt(){
  //read the DRT Json File
  this->version = this->drt[0] << 8 | this->drt[1];
}

//Low Level interface (These must be overridden by a subclass
int Nysa::write_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::read_periph_data(uint32_t dev_addr, uint32_t addr, uint8_t *buffer, uint32_t size){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::write_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::read_memory(uint32_t address, uint8_t *buffer, uint32_t size){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::wait_for_interrupts(uint32_t timeout, uint32_t *interrupts){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::ping(){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

int Nysa::crash_report(uint32_t *buffer){
  printf ("Error: Calling function that should be subclassed!\n");
  return -1;
}

//Helper Functions
int Nysa::write_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t data){
  //write to only one address in the peripheral address space
  printd("Entered\n");
  uint8_t d[4];
  d[0] = ((data >> 24) & 0xFF);
  d[1] = ((data >> 16) & 0xFF);
  d[2] = ((data >> 8)  & 0xFF);
  d[3] = ( data        & 0xFF);
  return this->write_periph_data(dev_addr, reg_addr, &d[0], 4);
}

int Nysa::read_register(uint32_t dev_addr, uint32_t reg_addr, uint32_t *data){
  //read from only one address in the peripheral address space
  printd("Entered\n");
  uint8_t d[4];
  uint32_t retval;
  retval = this->read_periph_data(dev_addr, reg_addr, &d[0], 4);
  CHECK_NYSA_ERROR("Error Reading Peripheral Data");
  //printf ("%02X %02X %02X %02X\n", d[0], d[1], d[2], d[3]);
  *data = (d[0] << 24 | d[1] << 16 | d[2] << 8 | d[3]);
  return 0;
}

int Nysa::set_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit){
  uint32_t reg;
  int retval = 0;
  printd("Entered\n");

  retval = this->read_register(dev_addr, reg_addr, &reg);
  CHECK_NYSA_ERROR("Error Reading Register");

  reg |= 1 << bit;
  retval = this->write_register(dev_addr, reg_addr, reg);
  CHECK_NYSA_ERROR("Error Writing Register");
  return 0;
}

int Nysa::clear_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit){
  uint32_t reg;
  int retval = 0;
  printd("Entered\n");
  retval = this->read_register(dev_addr, reg_addr, &reg);
  CHECK_NYSA_ERROR("Error Reading Register");
  reg &= (~(1 << bit));
  retval = this->write_register(dev_addr, reg_addr, reg);
  CHECK_NYSA_ERROR("Error Writing Register");
  return 0;
}

int Nysa::read_register_bit(uint32_t dev_addr, uint32_t reg_addr, uint8_t bit, bool * value){
  uint32_t reg;
  int retval = 0;
  printd("Entered\n");
  retval = this->read_register(dev_addr, reg_addr, &reg);
  CHECK_NYSA_ERROR("Error Reading Register");
  reg &= (1 << bit);
  if (reg > 0){
    *value = true;
  }
  else {
    *value = false;
  }
  return 0;
}

//DRT
int Nysa::pretty_print_drt(){
  return -1;
}

int Nysa::read_drt(){
  uint8_t * buffer = new uint8_t [32];
  uint32_t len = 1 * 32;
  int32_t retval;
  //We don't know the total size of the DRT so only look at the fist Block (32 bytes)
  retval = this->read_periph_data(0, 0, buffer, len);
  if (retval < 0){
    printf ("%s(): Failed to read peripheral data\n", __func__);
    return -1;
  }
  //Found out how many devices are in the DRT
  this->num_devices = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7];
  printf ("There are: %d devices\n", this->num_devices);
  delete(buffer);

  //Calculate the buffer size ( + 1 to read the DRT again)
  len = (this->num_devices + 1) * 32;
  printf ("Length of read: %d\n", len);

  this->drt = new uint8_t [len];
  retval = this->read_periph_data(0, 0, this->drt, len);
  this->parse_drt();
  return 0;
}

int Nysa::get_drt_version(){
  if (this->drt == NULL){
    return -1;
  }
  return this->version;
}

int Nysa::get_drt_device_count(){
  if (this->drt == NULL){
    return -1;
  }
  return this->num_devices;
}
uint32_t Nysa::get_drt_device_type(uint32_t index){
  uint32_t pos = 0;
  uint32_t type;
  if (this->drt == NULL){
    //XXX: Error handling
    return 0;
  }
  if (index == 0){
    //DRT has no type
    //XXX: Error handling
    return 0;
  }
  if (index > this->num_devices){
    //Out of range
    //XXX: Error handling
    return 0;
  }
  //Start of the device in question
  pos = (index) * 32;
  //Go to the start of the type
  type = this->drt[pos] << 24 | this->drt[pos + 1] << 16 | this->drt[pos + 2] << 8| this->drt[pos + 3];
  return type;
}
uint32_t Nysa::get_drt_device_size(uint32_t index){
  uint32_t size = 0;
  uint32_t pos = 0;
  if (this->drt == NULL){
    //XXX: Error handling
    return 0;
  }
  if (index == 0){
    //DRT has no type
    //XXX: Error handling
    return 0;
  }
  if (index > this->num_devices){
    //Out of range
    //XXX: Error handling
    return 0;
  }
  //Start of the device in question
  pos = (index) * 32;
  pos += 12;
  //Go to the start of the type
  size = this->drt[pos] << 24 | this->drt[pos + 1] << 16 | this->drt[pos + 2] << 8 | this->drt[pos + 3];
  return size;
}

uint32_t Nysa::get_drt_device_addr(uint32_t index){
  uint32_t pos = 0;
  uint32_t addr;
  if (this->drt == NULL){
    //XXX: Error handling
    return 0;
  }
  if (index == 0){
    //DRT has no type
    //XXX: Error handling
    return 0;
  }
  if (index > this->num_devices){
    //Out of range
    //XXX: Error handling
    return 0;
  }
  //Start of the device in question
  pos = (index) * 32;
  pos += 8;
  //Go to the start of the type
  addr = this->drt[pos] << 24 | this->drt[pos + 1] << 16 | this->drt[pos + 2] << 8 | this->drt[pos + 3];
  return addr;

}
int Nysa::get_drt_device_flags(uint32_t index, uint16_t *nysa_flags, uint16_t *dev_flags){
  uint32_t pos = 0;
  if (this->drt == NULL){
    return -1;
  }
  if (index == 0){
    //DRT has no type
    return -2;
  }
  if (index > this->num_devices){
    //Out of range
    return -3;
  }
  //Start of the device in question
  pos = (index) * 32;
  pos += 4;
  //Go to the start of the type
  *nysa_flags = (this->drt[pos] << 8)      | (this->drt[pos + 1]);
  *dev_flags  = (this->drt[pos + 2] << 8)  | (this->drt[pos + 3]);


}
bool Nysa::is_memory_device(uint32_t index){
  int retval = 0;
  uint16_t nysa_flags;
  uint16_t dev_flags;
  retval = this->get_drt_device_flags(index, &nysa_flags, &dev_flags);
  if (nysa_flags & 0x01){
    return true;
  }
  return false;
}

int Nysa::pretty_print_crash_report(){
  return -1;
}


uint32_t Nysa::find_device(uint32_t device_type, uint32_t subtype, uint32_t id){
  for (int i = 1; i  < this->get_drt_device_count() + 1; i ++){
    if (this->get_drt_device_type(i) == device_type){
      if (subtype > 0){
        //User has specified a subtype ID
        //XXX: Not implemented yet
      }
      if (id > 0){
        //User has specified an implimentation specific identification
        //XXX: Not implemented yet
      }
      return i;
    }
  }
  return 0;
}
