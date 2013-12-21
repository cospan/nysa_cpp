#ifndef __NH_LCD_480_272_HPP__
#define __NH_LCD_480_272_HPP__

#define LCD_DEVICE_ID 13
#define NH_LCD_480_272_DEVICE_SUB_ID 0

#define NH_LCD_480_272_HEIGHT 480
#define NH_LCD_480_272_WIDTH 272

#define BLOCKING true

#include "driver.hpp"

const static uint32_t get_nh_lcd_480_272_type(){
  return (uint32_t) NH_LCD_480_272_DEVICE_SUB_ID;
}

class NH_LCD_480_272 : public Driver {

  const static uint32_t DMA_BASE0                   = 0x00000000;
  const static uint32_t DMA_BASE1                   = 0x00010000;

  const static uint32_t DMA_SIZE                    = ((NH_LCD_480_272_HEIGHT) * (NH_LCD_480_272_WIDTH));

  //LCD Constants
  const static uint16_t HSYNC_TOTAL                 =  525;
  const static uint16_t HBLANK                      =  68;
  const static uint16_t HSYNC_PULSE                 =  40;
  const static uint16_t HSYNC_PULSE_START           =  0;

  const static uint16_t VSYNC_TOTAL                 =  360;
  const static uint16_t VBLANK                      =  12;
  const static uint16_t VSYNC_PULSE                 =  9;
  const static uint16_t VSYNC_PULSE_START           =  0;

  const static uint16_t COLUMN_START                =  0;
  const static uint16_t COLUMN_END                  =  NH_LCD_480_272_WIDTH;
  const static uint16_t PAGE_START                  =  0;
  const static uint16_t PAGE_END                    =  NH_LCD_480_272_HEIGHT;

  //MCU Addresses
  const static uint8_t MEM_ADR_NOP                  =  0x00;
  const static uint8_t MEM_ADR_RESET                =  0x01;
  const static uint8_t MEM_ADR_PWR_MODE             =  0x0A;
  const static uint8_t MEM_ADR_ADR_MODE             =  0x0B;
  const static uint8_t MEM_ADR_DISP_MODE            =  0x0D;
  const static uint8_t MEM_ADR_GET_TEAR_EF          =  0x0E;
  const static uint8_t MEM_ADR_ENTER_SLEEP_MODE     =  0x10;
  const static uint8_t MEM_ADR_EXIT_SLEEP_MODE      =  0x11;
  const static uint8_t MEM_ADR_ENTER_PARTIAL_MODE   =  0x12;
  const static uint8_t MEM_ADR_EXIT_PARTIAL_MODE    =  0x13;
  const static uint8_t MEM_ADR_EXIT_INVERT_MODE     =  0x20;
  const static uint8_t MEM_ADR_ENTER_INVERT_MODE    =  0x21;
  const static uint8_t MEM_ADR_SET_GAMMA_CURVE      =  0x26;
  const static uint8_t MEM_ADR_SET_DISPLAY_OFF      =  0x28;
  const static uint8_t MEM_ADR_SET_DISPLAY_ON       =  0x29;
  const static uint8_t MEM_ADR_SET_COLUMN_ADR       =  0x2A;
  const static uint8_t MEM_ADR_SET_PAGE_ADR         =  0x2B;
  const static uint8_t MEM_ADR_WRITE_MEM_START      =  0x2C;
  const static uint8_t MEM_ADR_READ_MEM_START       =  0x2E;
  const static uint8_t MEM_ADR_SET_PARTIAL_AREA     =  0x30;
  const static uint8_t MEM_ADR_SET_SCROLL_AREA      =  0x33;
  const static uint8_t MEM_ADR_SET_TEAR_OFF         =  0x34;
  const static uint8_t MEM_ADR_SET_TEAR_ON          =  0x35;
  const static uint8_t MEM_ADR_SET_ADR_MODE         =  0x36;
  const static uint8_t MEM_ADR_SET_SCROLL_START     =  0x37;
  const static uint8_t MEM_ADR_EXIT_IDLE_MODE       =  0x38;
  const static uint8_t MEM_ADR_ENTER_IDLE_MODE      =  0x39;
  const static uint8_t MEM_ADR_SET_PIXEL_FORMAT     =  0x3A;
  const static uint8_t MEM_ADR_WRITE_MEM_CONT       =  0x3C;
  const static uint8_t MEM_ADR_READ_MEM_CONT        =  0x3E;
  const static uint8_t MEM_ADR_SET_TEAR_SCANLINE    =  0x44;
  const static uint8_t MEM_ADR_GET_SCANLINE         =  0x45;
  const static uint8_t MEM_ADR_READ_DDB             =  0xA1;
  const static uint8_t MEM_ADR_SET_LCD_MODE         =  0xB0;
  const static uint8_t MEM_ADR_GET_LCD_MODE         =  0xB1;
  const static uint8_t MEM_ADR_SET_HORIZ_PERIOD     =  0xB4;
  const static uint8_t MEM_ADR_GET_HORIZ_PERIOD     =  0xB5;
  const static uint8_t MEM_ADR_SET_VERT_PERIOD      =  0xB6;
  const static uint8_t MEM_ADR_GET_VERT_PERIOD      =  0xB7;
  const static uint8_t MEM_ADR_SET_GPIO_CONF        =  0xB8;
  const static uint8_t MEM_ADR_GET_GPIO_CONF        =  0xB9;
  const static uint8_t MEM_ADR_SET_GPIO_VAL         =  0xBA;
  const static uint8_t MEM_ADR_GET_GPIO_STATUS      =  0xBB;
  const static uint8_t MEM_ADR_SET_POST_PROC        =  0xBC;
  const static uint8_t MEM_ADR_GET_POST_PROC        =  0xBD;
  const static uint8_t MEM_ADR_SET_PWM_CONF         =  0xBE;
  const static uint8_t MEM_ADR_GET_PWM_CONF         =  0xBF;
  const static uint8_t MEM_ADR_SET_LCD_GEN0         =  0xC0;
  const static uint8_t MEM_ADR_GET_LCD_GEN0         =  0xC1;
  const static uint8_t MEM_ADR_SET_LCD_GEN1         =  0xC2;
  const static uint8_t MEM_ADR_GET_LCD_GEN1         =  0xC3;
  const static uint8_t MEM_ADR_SET_LCD_GEN2         =  0xC4;
  const static uint8_t MEM_ADR_GET_LCD_GEN2         =  0xC5;
  const static uint8_t MEM_ADR_SET_LCD_GEN3         =  0xC6;
  const static uint8_t MEM_ADR_GET_LCD_GEN3         =  0xC7;
  const static uint8_t MEM_ADR_SET_GPIO0_ROP        =  0xC8;
  const static uint8_t MEM_ADR_GET_GPIO0_ROP        =  0xC9;
  const static uint8_t MEM_ADR_SET_GPIO1_ROP        =  0xCA;
  const static uint8_t MEM_ADR_GET_GPIO1_ROP        =  0xCB;
  const static uint8_t MEM_ADR_SET_GPIO2_ROP        =  0xCC;
  const static uint8_t MEM_ADR_GET_GPIO2_ROP        =  0xCD;
  const static uint8_t MEM_ADR_SET_GPIO3_ROP        =  0xCE;
  const static uint8_t MEM_ADR_GET_GPIO3_ROP        =  0xCF;
  const static uint8_t MEM_ADR_SET_DBC_CONF         =  0xD0;
  const static uint8_t MEM_ADR_GET_DBC_CONF         =  0xD1;
  const static uint8_t MEM_ADR_SET_DBC_TH           =  0xD4;
  const static uint8_t MEM_ADR_GET_DBC_TH           =  0xD5;
  const static uint8_t MEM_ADR_SET_PLL              =  0xE0;
  const static uint8_t MEM_ADR_SET_PLL_MN           =  0xE2;
  const static uint8_t MEM_ADR_GET_PLL_MN           =  0xE3;
  const static uint8_t MEM_ADR_GET_PLL_STATUS       =  0xE4;
  const static uint8_t MEM_ADR_SET_LSHIFT_FREQ      =  0xE6;
  const static uint8_t MEM_ADR_GET_LSHIFT_FREQ      =  0xE7;
  const static uint8_t MEM_ADR_SET_PIX_DAT_INT      =  0xF0;
  const static uint8_t MEM_ADR_GET_PIX_DAT_INT      =  0xF1;

  private:
    bool debug;
    DMA * dma;

    void write_lcd_command(uint8_t command, uint32_t data_count = 0, uint8_t *data = NULL);
    void read_lcd_command(uint8_t command, uint32_t data_count, uint8_t *data);

    void enable_tearing(bool enable);
    void enable_backlight(bool enable);
    void override_write_enable(bool enable);
    void enable_chip_select(bool enable);
    void pixel_frequency_to_array(double pll_clock = 100, double pixel_freq = 5.3, uint8_t *buffer = NULL);
    void setup();

  public:
    NH_LCD_480_272(Nysa *nysa, uint32_t dev_addr, bool debug = false);
    ~NH_LCD_480_272();

    //Control
    void start();
    void stop();

    uint32_t get_buffer_size();
    uint32_t get_image_width();
    uint32_t get_image_height();

    void reset();


    //Data Transfer
    void dma_write(uint8_t *buffer);


};



#endif //__NH_LCD_480_272_HPP__
