#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <math.h>
#include "nh_lcd_480_272.hpp"
#include "print_colors.hpp"

//LCD Constants

void print_debug(const char *name , bool writing, uint8_t* mode, uint32_t length);

enum REGISTERS{
  REG_CONTROL               = 0,
  REG_STATUS                = 1,
  REG_COMMAND_DATA          = 2,
  REG_PIXEL_COUNT           = 3,
  REG_MEM_0_BASE            = 4,
  REG_MEM_0_SIZE            = 5,
  REG_MEM_1_BASE            = 6,
  REG_MEM_1_SIZE            = 7
};

enum CONTROL_REG {
  CONTROL_ENABLE            = 0,
  CONTROL_ENABLE_INTERRUPT  = 1,
  CONTROL_COMMAND_MODE      = 2,
  CONTROL_BACKLIGHT_ENABLE  = 3,
  CONTROL_RESET_DISPLAY     = 4,
  CONTROL_COMMAND_WRITE     = 5,
  CONTROL_COMMAND_READ      = 6,
  CONTROL_COMMAND_PARAMETER = 7,
  CONTROL_WRITE_OVERRIDE    = 8,
  CONTROL_CHIP_SELECT       = 9,
  CONTROL_ENABLE_TEARING    = 10
};

enum STATUS_REG {
  STATUS_0_EMPTY            = 0,
  STATUS_1_EMPTY            = 1
};

NH_LCD_480_272::NH_LCD_480_272(Nysa *nysa, uint32_t dev_addr, bool debug) : Driver(nysa, debug){
  this->set_device_id(LCD_DEVICE_ID);
  this->find_device();
  this->set_device_sub_id(NH_LCD_480_272_DEVICE_SUB_ID);
  this->debug = debug;
  if (this->debug){
    printf ("Setting up DMA Write\n");
    printf ("\tDMA Base 0:      0x%08X\n", DMA_BASE0);
    printf ("\tDMA Base 1:      0x%08X\n", DMA_BASE1);
    printf ("\tDMA Size:        0x%08X\n", DMA_SIZE);
    printf ("\tDMA Size:        %d\n", DMA_SIZE);
    printf ("\tReg Status:      0x%08X\n", REG_STATUS);
    printf ("\tReg Mem 0 Base:  0x%08X\n", REG_MEM_0_BASE);
    printf ("\tReg Mem 0 Size:  0x%08X\n", REG_MEM_0_SIZE);
    printf ("\tReg Mem 1 base:  0x%08X\n", REG_MEM_1_BASE);
    printf ("\tReg Mem 1 Size:  0x%08X\n", REG_MEM_1_SIZE);
    printf ("\tBlocking:        %d\n", BLOCKING);
    printf ("\tStrategy:        %d\n", CADENCE);
  }

  this->dma                   = new DMA(nysa, this, dev_addr, debug);
  this->dma->setup_write(     DMA_BASE0,
                              DMA_BASE1,
                              DMA_SIZE,
                              REG_STATUS,
                              REG_MEM_0_BASE,
                              REG_MEM_0_SIZE,
                              REG_MEM_1_BASE,
                              REG_MEM_1_SIZE,
                              BLOCKING,
                              CADENCE);
  this->dma->set_status_bits( 0,
                              0,
                              STATUS_0_EMPTY,
                              STATUS_1_EMPTY);

  if (this->debug){
    printf ("LCD Register Dump\n");
    printf ("BASE 0: 0x%08X\n", this->read_register(REG_MEM_0_BASE));
    printf ("BASE 1: 0x%08X\n", this->read_register(REG_MEM_1_BASE));
  }

}
NH_LCD_480_272::~NH_LCD_480_272(){
  delete(this->dma);
}

void NH_LCD_480_272::setup(){
  uint8_t buffer[256];
  uint8_t lcd_width_hb = (((NH_LCD_480_272_WIDTH - 1) >> 8) & 0xFF);
  uint8_t lcd_width_lb = ( (NH_LCD_480_272_WIDTH - 1)       & 0xFF);

  uint8_t lcd_height_hb = (((NH_LCD_480_272_HEIGHT - 1) >> 8) & 0xFF);
  uint8_t lcd_height_lb = ((NH_LCD_480_272_HEIGHT - 1) & 0xFF);

  //Horizontal
  uint8_t hsync_hb = (((HSYNC_TOTAL) >> 8) & 0xFF);
  uint8_t hsync_lb = ((HSYNC_TOTAL) & 0xFF);

  uint8_t hblank_hb = (((HBLANK) >> 8) & 0xFF);
  uint8_t hblank_lb = ((HBLANK) & 0xFF);

  uint8_t hsync_pulse_start_hb = (((HSYNC_PULSE_START) >> 8) & 0xFF);
  uint8_t hsync_pulse_start_lb = ((HSYNC_PULSE_START) & 0xFF);

  //Vertical
  uint8_t vsync_hb = (((VSYNC_TOTAL) >> 8) & 0xFF);
  uint8_t vsync_lb = ((VSYNC_TOTAL) & 0xFF);

  uint8_t vblank_hb = (((VBLANK) >> 8) & 0xFF);
  uint8_t vblank_lb = ((VBLANK) & 0xFF);

  uint8_t vsync_pulse_start_hb = (((VSYNC_PULSE_START) >> 8) & 0xFF);
  uint8_t vsync_pulse_start_lb = ((VSYNC_PULSE_START) & 0xFF);

  uint8_t column_start_hb = (((COLUMN_START) >> 8) & 0xFF);
  uint8_t column_start_lb = ((COLUMN_START) & 0xFF);

  uint8_t column_end_hb = (((COLUMN_END - 1) >> 8) & 0xFF);
  uint8_t column_end_lb = ((COLUMN_END - 1) & 0xFF);

  uint8_t page_start_hb = (((PAGE_START) >> 8) & 0xFF);
  uint8_t page_start_lb = ((PAGE_START) & 0xFF);

  uint8_t page_end_hb = (((PAGE_END - 1) >> 8) & 0xFF);
  uint8_t page_end_lb = ((PAGE_END - 1) & 0xFF);

  //Reset the LCD
  this->enable_chip_select(true); printd("Enable Chip Select\n");
  this->start();                  printd("Enable the interrupts and core\n");
  this->enable_backlight(true);   printd("Enable Backlight\n");
  this->override_write_enable(true);  printd("Assert Write enable\n");
  this->reset();                  printd("Reset the LCD Core\n");
  this->override_write_enable(false);  printd("Deassert Write Enable\n");

  /*
  if (this->debug){
    uint8_t pwr_mode;
    this->read_lcd_command(MEM_ADR_PWR_MODE, 1, &pwr_mode);
    print_debug("MEM_ADR_PWR_MODE", false, &pwr_mode, 1);
  }
  */
  //Soft Reset the MCU
  this->write_lcd_command(MEM_ADR_RESET); printd("Reset LCD Core\n");
  print_debug("MEM_ADR_RESET", true, NULL, 0);
  usleep(500000);                           printd("Sleep for 500mS\n");
  this->write_lcd_command(MEM_ADR_RESET);
  print_debug("MEM_ADR_RESET", true, NULL, 0); printd("Reset LCD Core\n");
  usleep(200000);                           printd("Sleep for 200mS\n");
  //Start the PLL
  buffer[0] = 0x01;
  this->write_lcd_command(MEM_ADR_SET_PLL, 1, buffer);  printd("Start the PLL\n");
  print_debug("MEM_ADR_SET_PLL", true, buffer, 1);
  usleep(100000);                           printd("Sleep for 100mS\n");
  //Lock the PLL
  buffer[0] = 0x03;
  this->write_lcd_command(MEM_ADR_SET_PLL, 1, buffer);  printd("Lock the PLL\n");
  print_debug("MEM_ADR_SET_PLL", true, buffer, 1);

  /*
  if (this->debug){
    this->read_lcd_command(MEM_ADR_GET_LCD_MODE, 7, buffer);
    print_debug("MEM_ADR_GET_LCD_MODE", false, buffer, 7);
  }
  */

  //Setup the LCD
  buffer[0] = 0x00;         //Set TFT Mode: 0x0C ??
  buffer[1] = 0x00;         //Set TFT Mode & Hsync + Vsync + DEN Mode
  buffer[2] = lcd_width_hb; //Set Horizontal Size High Byte
  buffer[3] = lcd_width_lb; //Set Horizontal Size Low Byte
  buffer[4] = lcd_height_hb;//Set Vertical Size High Byte
  buffer[5] = lcd_height_lb;//Set Vertical Size Low Byte
  buffer[6] = 0x00;         //Set even/odd line RGB sequency = RGB
  this->write_lcd_command(MEM_ADR_SET_LCD_MODE, 7, buffer); printd("Setup the LCD Mode\n");
  print_debug("MEM_ADR_SET_LCD_MODE", true, buffer, 7);

  /*
  if (this->debug){
    this->read_lcd_command(MEM_ADR_GET_LCD_MODE, 7, buffer);
    print_debug("MEM_ADR_GET_LCD_MODE", false, buffer, 7);
  }
  */

  //Set Pixel data I/F format = 8 bit
  buffer[0] = 0x00;
  this->write_lcd_command(MEM_ADR_SET_PIX_DAT_INT, 1, buffer); printd("Set the pixel buffer : 8bits\n");
  print_debug("MEM_ADR_SET_PIX_DAT_INT", true, buffer, 1);

  //Set RGB Format: 6 6 6
  buffer[0] = 0x60;
  this->write_lcd_command(MEM_ADR_SET_PIXEL_FORMAT, 1, buffer); printd("pixel format: 6:6:6\n");
  print_debug("MEM_ADR_SET_PIXEL_FORMAT", true, buffer, 1);

  //Setup PLL Frequency
  buffer[0] = 0x01;
  buffer[1] = 0x45;
  buffer[2] = 0x47;
  this->write_lcd_command(MEM_ADR_SET_LSHIFT_FREQ, 3, buffer); printd("set PLL Frequency\n");
  print_debug("MEM_ADR_SET_LSHIFT_FREQ", true, buffer, 3);

  //Setup Horizontal Behavior
  buffer[0] = hsync_hb;               //(high byte Set HSYNC Total Lines: 525
  buffer[1] = hsync_lb;               //(low byte Set HSYNC Total Lines: 525
  buffer[2] = hblank_hb;              //(high byte Set Horizonatal Blanking Period: 68
  buffer[3] = hblank_lb;              //(low byte Set Horizontal Blanking Period: 68
  buffer[4] = HSYNC_PULSE;            //Set horizontal balnking period 16 = 15 + 1
  buffer[5] = hsync_pulse_start_hb;   //(high byte Set Hsync pulse start position
  buffer[6] = hsync_pulse_start_lb;   //(low byte Set Hsync pulse start position
  this->write_lcd_command(MEM_ADR_SET_HORIZ_PERIOD, 7, buffer); printd("Set Horizontal Behavior\n");
  print_debug("MEM_ADR_SET_HORIZ_PERIOD", true, buffer, 7);


  //Setup Vertical Blanking Period
  buffer[0] = vsync_hb;               //(high byte Set Vsync total: 360
  buffer[1] = vsync_lb;               //(low byte Set vsync total: 360
  buffer[2] = vblank_hb;              //(high byte Set Vertical Blanking Period: 19
  buffer[3] = vblank_lb;              //(low byte Set Vertical Blanking Period: 19
  buffer[4] = VSYNC_PULSE;            //Vsync pulse: 8 = 7 + 1
  buffer[5] = vsync_pulse_start_hb;   //(high byte Set Vsync pusle start position
  buffer[6] = vsync_pulse_start_lb;   //(low byte Set Vsync pusle start position
  this->write_lcd_command(MEM_ADR_SET_VERT_PERIOD, 7, buffer); printd("Set vertical blanking period\n");
  print_debug("MEM_ADR_SET_VERT_PERIOD", true, buffer, 7);

  //Setup column address
  buffer[0] = column_start_hb; //(high byte) Set start column address: 0
  buffer[1] = column_start_lb; //(low byte) Set start column address: 0
  buffer[2] = column_end_hb;   //(high byte) Set end column address: 479
  buffer[3] = column_end_lb;   //(low byte) Set end column address: 479
  this->write_lcd_command(MEM_ADR_SET_COLUMN_ADR, 4, buffer);

  print_debug("MEM_ADR_SET_COLUMN_ADR", true, buffer, 4);


  //Setup Page Address
  buffer[0] = page_start_hb;   //(high byte Start page address: 0
  buffer[1] = page_start_lb;   //(low byte Start page address: 0
  buffer[2] = page_end_hb;     //(high byte end page address: 271
  buffer[3] = page_end_lb;     //(low byte end page address: 271
  this->write_lcd_command(MEM_ADR_SET_PAGE_ADR, 4, buffer); printd("Set Column Address\n");
  print_debug("MEM_ADR_SET_PAGE_ADR", true, buffer, 4);

  buffer[0] = 0x00;
  this->write_lcd_command(MEM_ADR_SET_ADR_MODE, 1, buffer); printd("Set Image Configuration\n");
  print_debug("MEM_ADR_SET_ADR_MODE", true, buffer, 1);

  //Setup Image Configuration
  this->write_lcd_command(MEM_ADR_EXIT_PARTIAL_MODE); printd("Disable partial Mode\n");
  print_debug("MEM_ADR_EXIT_PARTIAL_MODE", true, buffer, 0);
  this->write_lcd_command(MEM_ADR_EXIT_IDLE_MODE);  printd("Exit IDLE\n");
  print_debug("MEM_ADR_EXIT_IDLE_MODE", true, buffer, 0);
  this->write_lcd_command(MEM_ADR_SET_DISPLAY_ON);  printd("Display On\n");
  print_debug("MEM_ADR_SET_DISPLAY_ON", true, buffer, 0);

  //Setup the correct pixel count
  this->write_register(REG_PIXEL_COUNT,
                       (uint32_t)(NH_LCD_480_272_HEIGHT * NH_LCD_480_272_WIDTH));
  if (this->debug){
    printf ("%s(): Set Pixel Count to: 0x%08X\n",
            __func__,
            (NH_LCD_480_272_HEIGHT * NH_LCD_480_272_WIDTH));
  }

  //Enable Tearing
  buffer[0] = 0x00;
  this->write_lcd_command(MEM_ADR_SET_TEAR_ON, 1, buffer); printd("Enable tearing control\n");
  print_debug("MEM_ADR_SET_TEAR_ON", true, buffer, 1);
  this->enable_tearing(true);                   printd("Enable tearing in core\n");

}

void NH_LCD_480_272::enable_tearing(bool enable){
  if (enable){
    this->set_register_bit(REG_CONTROL, CONTROL_ENABLE_TEARING);
  }
  else {
    this->clear_register_bit(REG_CONTROL, CONTROL_ENABLE_TEARING);
  }

}
void NH_LCD_480_272::pixel_frequency_to_array(double pll_clock, double pixel_freq, uint8_t *buffer){
  uint32_t value = (uint32_t) ((pixel_freq / pll_clock) * pow(2, 20)) - 1;
  buffer[0] = ((value >> 16) & 0xFF);
  buffer[1] = ((value >> 8 ) & 0xFF);
  buffer[2] = ((value      ) & 0xFF);
}
void NH_LCD_480_272::enable_chip_select(bool enable){
  if (enable){
    this->set_register_bit(REG_CONTROL, CONTROL_CHIP_SELECT);
  }
  else {
    this->clear_register_bit(REG_CONTROL, CONTROL_CHIP_SELECT);
  }
}

void NH_LCD_480_272::override_write_enable(bool enable){
  if (enable){
    this->set_register_bit(REG_CONTROL, CONTROL_WRITE_OVERRIDE);
  }
  else {
    this->clear_register_bit(REG_CONTROL, CONTROL_WRITE_OVERRIDE);
  }
}
void NH_LCD_480_272::enable_backlight(bool enable){
  if (enable){
    this->set_register_bit(REG_CONTROL, CONTROL_BACKLIGHT_ENABLE);
  }
  else {
    this->clear_register_bit(REG_CONTROL, CONTROL_BACKLIGHT_ENABLE);
  }
}

void NH_LCD_480_272::write_lcd_command(uint8_t address, uint32_t data_count, uint8_t *data){
  //Go into control mode
  this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_MODE);
  //Tell the LCD command controller we are SENDING a command
  this->clear_register_bit(REG_CONTROL, CONTROL_COMMAND_PARAMETER);
  //Put the data in the register
  this->write_register(REG_COMMAND_DATA, address);
  //Writing!
  this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_WRITE);
  //If the data_count > 0:
  for (int i = 0; i < data_count; i++){
    this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_PARAMETER);
    this->write_register(REG_COMMAND_DATA, data[i]);
    this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_WRITE);
  }
  //Go back into data mode
  this->clear_register_bit(REG_CONTROL, CONTROL_COMMAND_MODE);
}
void NH_LCD_480_272::read_lcd_command(uint8_t address, uint32_t data_count, uint8_t *data){
  //Go into command mode
  this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_MODE);
  //Tell the LCD command controller we are sending the command
  this->clear_register_bit(REG_CONTROL, CONTROL_COMMAND_PARAMETER);
  //Put the daa in the register
  this->write_register(REG_COMMAND_DATA, address);
  //Writing
  this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_WRITE);
  //go through each of the incomming peices of data and put it in the data buffer
  for (int i = 0; i < data_count; i++){
    this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_PARAMETER);
    this->set_register_bit(REG_CONTROL, CONTROL_COMMAND_READ);
    data[i] = (uint8_t) this->read_register(REG_COMMAND_DATA);
  }
  //go back to data mode
  this->clear_register_bit(REG_CONTROL, CONTROL_COMMAND_MODE);
}

//Control
void NH_LCD_480_272::start(){
  this->set_register_bit(REG_CONTROL, CONTROL_ENABLE);
  this->set_register_bit(REG_CONTROL, CONTROL_ENABLE_INTERRUPT);
}
void NH_LCD_480_272::stop(){
  this->clear_register_bit(REG_CONTROL, CONTROL_ENABLE);
  this->clear_register_bit(REG_CONTROL, CONTROL_ENABLE_INTERRUPT);
  this->enable_backlight(false);
}

void NH_LCD_480_272::reset(){
  //printf ("%s(): Reset...\n", __func__);
  this->set_register_bit(REG_CONTROL, CONTROL_RESET_DISPLAY);
  usleep(30000);
  //printf ("%s(): Reset finished...\n", __func__);
  this->clear_register_bit(REG_CONTROL, CONTROL_RESET_DISPLAY);
}


uint32_t NH_LCD_480_272::get_buffer_size(){
  return DMA_SIZE;
}

//Data Transfer
void NH_LCD_480_272::dma_write(uint8_t *buffer){
  this->dma->write(buffer);
}

void print_debug(const char* name, bool writing, uint8_t* mode, uint32_t length){

  if (writing){
    printf (P_CYAN);
    printf ("LCD Write: %s\n", name);
    printf (P_BLUE);
    if (length > 0){
      printf ("\t");
    }
    for (unsigned i = 0; i < length; i++){
      printf ("%02X ", mode[i]);
    }
    if (length > 0){
      printf ("\n");
    }
  }
  else{
    printf (P_MAGENTA);
    printf ("LCD Read: %s\n", name);
    printf (P_YELLOW);
    if (length > 0){
      printf ("\t");
    }
    for (unsigned i = 0; i < length; i++){
      printf ("%02X ", mode[i]);
    }
    if (length > 0){
      printf ("\n");
    }
  }
  printf (P_NORMAL);
}

