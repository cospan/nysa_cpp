#include <stdio.h>
#include "arduino.hpp"
#include "gpio.hpp"

enum GPIO_REGISTERS{
  GPIO_PORT          = 0,
  GPIO_OUTPUT_ENABLE = 1,
  INTERRUPTS         = 2,
  INTERRUPTS_ENABLE  = 3,
  INTERRUPTS_EDGE    = 4
};

GPIO::GPIO(Nysa *nysa, uint32_t dev_addr, bool debug) : Driver(nysa, debug){
  this->debug = debug;
  this->set_device_id(GPIO_DEVICE_ID);
  this->set_device_sub_id(GPIO_DEVICE_SUB_ID);
  this->find_device();
}

GPIO::~GPIO(){
}
//Arduino Compatible functions
void GPIO::pinMode(uint32_t pin, uint32_t direction){	
  
  if (direction == 0){
    this->clear_output_mask_bit(pin);
  }
  else{
    this->set_output_mask_bit(pin);
  }
}
void GPIO::digitalWrite(uint32_t bit, uint32_t value){	
  
  uint32_t gpios;
  gpios = this->get_gpios();
  if (value > 0){
    this->set_register_bit(GPIO_PORT, bit);
  }
  else {
    this->clear_register_bit(GPIO_PORT, bit);
    gpios &= (~(1 << bit));
  }
  this->set_gpios(gpios);
}
bool GPIO::digitalRead(uint32_t bit){	
  uint32_t gpios;
  gpios = this->get_gpios();
  return ((gpios & (1 << bit)) > 0);
}

void GPIO::toggle(uint32_t bit){
  bool value;
  value = this->read_register_bit(GPIO_PORT, bit);
  if (value){
    this->clear_register_bit(GPIO_PORT, bit);
  }
  else {
    this->set_register_bit(GPIO_PORT, bit);
  }
}

//Public Functions
void GPIO::set_gpios(uint32_t gpios){	
  
  this->write_register(GPIO_PORT, gpios);
}
uint32_t GPIO::get_gpios(){
  
  return this->read_register(GPIO_PORT);
}

//Setting bits to inputs or outputs
void GPIO::set_output_mask(uint32_t output){	
  
  this->write_register(GPIO_OUTPUT_ENABLE, output);
}
uint32_t GPIO::get_output_mask(){	
  
  return this->read_register(GPIO_OUTPUT_ENABLE);
}
void GPIO::set_output_mask_bit(uint32_t bit){	
  
  //
  this->set_register_bit(GPIO_OUTPUT_ENABLE, bit);
}
void GPIO::clear_output_mask_bit(uint32_t bit){	
  
  //
  this->clear_register_bit(GPIO_OUTPUT_ENABLE, bit);
}

bool GPIO::get_output_mask_bit(uint32_t bit){	
  

  uint32_t mask;
  mask = this->get_output_mask();
  return (mask & (1 << bit));
}

//Interrupt Enable
void GPIO::set_interrupts_enable(uint32_t interrupts_enable){	
  this->write_register(INTERRUPTS_ENABLE, interrupts_enable);
}
uint32_t GPIO::get_interrupts_enable(){	
  return this->read_register(INTERRUPTS_ENABLE);
}
void GPIO::set_interrupts_enable_bit(uint32_t bit){	
  uint32_t enable;
  enable = this->get_interrupts_enable();
  enable |= (1 << bit);
  this->set_interrupts_enable(enable);
}
void GPIO::clear_interrupts_enable_bit(uint32_t bit){
  uint32_t enable;
  enable = this->get_interrupts_enable();
  enable &= (~(1 << bit));
  this->set_interrupts_enable(enable);
}
bool GPIO::get_interrupts_enable_bit(uint32_t bit){	
  uint32_t enable;
  enable = this->get_interrupts_enable();
  return ((enable & (1 << bit)) > 0);

}


//Interrupt Edge
void GPIO::set_interrupts_edge_mask(uint32_t edge_mask){	
  this->write_register(INTERRUPTS_EDGE, edge_mask);
}
uint32_t GPIO::get_interrupts_edge_mask(){	
  return this->read_register(INTERRUPTS_EDGE);
}
void GPIO::set_interrupts_edge_mask_bit(uint32_t bit){	
  uint32_t edge;
  edge = this->get_interrupts_edge_mask();
  edge |= (1 << bit);
  this->set_interrupts_edge_mask(edge);
}
void GPIO::clear_interrupts_edge_mask_bit(uint32_t bit){
  uint32_t edge;
  edge = this->get_interrupts_edge_mask();
  edge &= (~(1 << bit));
  this->set_interrupts_edge_mask(edge);
}
bool GPIO::get_interrupts_edge_mask_bit(uint32_t bit){	
  uint32_t edge;
  edge = this->get_interrupts_edge_mask();
  return ((edge & (1 << bit)) > 0);
}

//Interrupts
uint32_t GPIO::get_interrupts(){	
  return this->read_register(INTERRUPTS);
}

