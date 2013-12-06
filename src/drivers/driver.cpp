#include "driver.hpp"
#include <stdio.h>
#include <string.h>

/*
 * Help users Identify return values
 */
enum MESSAGE {
  DEVICE_ID_NOT_SET       = -4,
  FAILED_TO_READ_DRT      = -3,
  NYSA_NOT_FOUND          = -2,
  DEVICE_NOT_FOUND        = -1,
  SUCCESS                 = 0,
  MULTIPLE_DEVICE_FOUND   = 1,
  LAST_ENTRY              = 0xFFFF
};


#ifdef __cplusplus
extern "C" {
#endif

struct _message_struct_t {
  int message_index;
  const char * message_string;
};

typedef struct _message_struct_t message_struct_t;
static message_struct_t driver_messages[] = {

  {DEVICE_ID_NOT_SET      , "Device ID is not set"                               },
  {FAILED_TO_READ_DRT     , "Failed to read DRT"                                 },
  {NYSA_NOT_FOUND         , "Nysa Not Found"                                     },
  {DEVICE_NOT_FOUND       , "Device Not Found"                                   },
  {SUCCESS                , "Success"                                            },
  {MULTIPLE_DEVICE_FOUND  , "Multiple Devces with the same ID or sub ID's Found" },
  {LAST_ENTRY             , "Error not found"                                    }

};

#ifdef __cplusplus
}
#endif


Driver::Driver(Nysa *nysa, bool debug){
  this->n = nysa;
  this->debug = debug;

  this->dev_index = 0;
  this->id = 0;
  this->sub_id = 0;
  this->unique_id = 0;
  this->message_id = 0;
  this->error = 0;
}
Driver::~Driver(){
}

static const char * driver_get_message(int retval){
  int i = 0;
  while (driver_messages[i].message_index != LAST_ENTRY){
    if (driver_messages[i].message_index == retval){
      return driver_messages[i].message_string;
    }
  }
  return driver_messages[i].message_string;
}

void Driver::set_unique_id(uint16_t id){
  this->unique_id = id;
}

//Find the device set up in the constructor
int Driver::find_device(){

  if (n == NULL){

    return NYSA_NOT_FOUND;
  }
  if (this->n->get_drt_device_count() == 0){
    this->n->read_drt();
    if (this->n->get_drt_device_count() == 0){
      this->error = FAILED_TO_READ_DRT;
      throw FAILED_TO_READ_DRT;
    }
  }
  for (int i = 1; i < this->n->get_drt_device_count() + 1; i++){
    if (this->n->get_drt_device_type(i) == this->id){
      //XXX: Sub ID Not implemented yet
      //XXX: Unique ID not implemented Yet
      this->dev_index = i;
      return SUCCESS;
    }
  }
  this->error = DEVICE_NOT_FOUND;
  return DEVICE_NOT_FOUND;
}
void Driver::set_device_id(uint16_t id){
  this->id = id;
}
void Driver::set_device_sub_id(uint16_t sub_id){
  this->sub_id = sub_id;
}

/*
 * Initialize your device after finding it on the bus
 */
int Driver::open(){         //Initialized a device that is found inside the DRT
  return -1;
}
int Driver::close(){        //Clean up the device
  return -1;
}

//Nysa Functions
void Driver::write_periph_data(uint32_t addr, uint8_t *buffer, uint32_t size){
  if (this->dev_index == 0){
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
  }
  this->error = this->n->write_periph_data(this->dev_index, addr, buffer, size);
  if (this->error != SUCCESS){
    throw this->error;
  }
}
void Driver::read_periph_data(uint32_t addr, uint8_t *buffer, uint32_t size){
  if (this->dev_index == 0){
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
  }
  this->error = this->n->read_periph_data(this->dev_index, addr, buffer, size);
  if (this->error != SUCCESS){
    throw this->error;
  }

}

void Driver::write_register(uint32_t reg_addr, uint32_t data){
  if (this->dev_index == 0){
    printd("dev index = 0\n");
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
  }
  this->error = this->n->write_register(this->dev_index, reg_addr, data);
  if (this->error != SUCCESS){
    throw this->error;
  }

}
uint32_t Driver::read_register(uint32_t reg_addr){
  uint32_t data;
  if (this->dev_index == 0){
    printd("dev index = 0\n");
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
  }
  this->error = this->n->read_register(this->dev_index, reg_addr, &data);
  if (this->error != SUCCESS){
    throw this->error;
  }

  return data;
}

void Driver::set_register_bit(uint32_t reg_addr, uint8_t bit){
  printd("Entered\n");
  if (this->dev_index == 0){
    printd("dev index = 0\n");
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
    return;
  }
  this->error = this->n->set_register_bit(this->dev_index, reg_addr, bit);
}
void Driver::clear_register_bit(uint32_t reg_addr, uint8_t bit){
  if (this->dev_index == 0){
    printd("dev index = 0\n");
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
    return;
  }
  this->error = this->n->clear_register_bit(this->dev_index, reg_addr, bit);
  if (this->error != SUCCESS){
    throw this->error;
  }

}
bool Driver::read_register_bit(uint32_t reg_addr, uint8_t bit){
  bool value;
  if (this->dev_index == 0){
    printd("dev index = 0\n");
    this->error = DEVICE_ID_NOT_SET;
    throw DEVICE_ID_NOT_SET;
  }

  this->error = this->n->read_register_bit(this->dev_index, reg_addr, bit, &value);
  if (this->error != SUCCESS){
    throw this->error;
  }

  return value;
}

