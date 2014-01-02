#ifndef __LCD_HPP__
#define __LCD_HPP__

#define LCD_DEVICE_ID 15
#include "driver.hpp"

const static uint32_t get_lcd_type(){
  return (uint32_t) LCD_DEVICE_ID;
}

#endif //__LCD_HPP__
