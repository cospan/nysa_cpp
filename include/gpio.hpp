#ifndef __GPIO_DRIVER_HPP__
#define __GPIO_DRIVER_HPP__

#define GPIO_DEVICE_ID 1
#define GPIO_DEVICE_SUB_ID 0

#include "driver.hpp"
static uint32_t get_gpio_device_type(){
  return (uint32_t) GPIO_DEVICE_ID;
}

class GPIO : protected Driver {

  private:
    bool debug;

  public:
    GPIO(Nysa *nysa, uint32_t dev_addr, bool debug = false);
    ~GPIO();

    //Arduino Compatible functions
    void pinMode(uint32_t pin, uint32_t direction);
    void digitalWrite(uint32_t bit, uint32_t value);
    bool digitalRead(uint32_t pin);
    void toggle(uint32_t pin);

    //Public Functions
    void set_gpios(uint32_t gpios);
    uint32_t get_gpios();

    //Setting bits to inputs or outputs
    void set_output_mask(uint32_t output);
    uint32_t get_output_mask();
    void set_output_mask_bit(uint32_t bit);
    void clear_output_mask_bit(uint32_t bit);
    bool get_output_mask_bit(uint32_t bit);

    //Interrupt Enable
    void set_interrupts_enable(uint32_t interrupts);
    uint32_t get_interrupts_enable();
    void set_interrupts_enable_bit(uint32_t bit);
    void clear_interrupts_enable_bit(uint32_t bit);
    bool get_interrupts_enable_bit(uint32_t bit);

    //Interrupt Edge
    void set_interrupts_edge_mask(uint32_t mask);
    uint32_t get_interrupts_edge_mask();
    void set_interrupts_edge_mask_bit(uint32_t bit);
    void clear_interrupts_edge_mask_bit(uint32_t bit);
    bool get_interrupts_edge_mask_bit(uint32_t bit);

    uint32_t get_interrupts();
};

#endif //__GPIO_DRIVER_HPP__

