#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include "ftdi.hpp"
#include "dionysus.hpp"
#include "gpio.hpp"
#include "arduino.hpp"
#include "dma_demo_reader.hpp"
#include "dma_demo_writer.hpp"
#include "nh_lcd_480_272.hpp"
#include "print_colors.hpp"

#define PROGRAM_NAME "dionysus-nysa-test"


#define MEMORY_TEST false
//5 seconds
#define GPIO_TEST_WAIT 10
#define COLOR_BUFFER_COUNT 4

using namespace Ftdi;

#define DEFAULT_ARGUMENTS     \
{                             \
  .vendor = DIONYSUS_VID,     \
  .product = DIONYSUS_PID,    \
  .debug = false              \
}

struct arguments {
  int vendor;
  int product;
  bool debug;
  bool leds;
  bool buttons;
  bool memory;
  bool dma_read;
  bool dma_write;
  bool lcd_test;
};

static double TimevalDiff(const struct timeval *a, const struct timeval *b){
    return (a->tv_sec - b->tv_sec) + 1e-6 * (a->tv_usec - b->tv_usec);
}

static void usage (int exit_status){
  fprintf (exit_status == EXIT_SUCCESS ? stdout : stderr,
      "\n"
      P_GRAY
      "USAGE: %s [-v <vendor>] [-p <product>] [-d] [-12m]\n"
      P_NORMAL
      "\n"
      "Options:\n"
      P_CYAN
      "-h, --help\n"
      "\tPrints this helpful message\n"
      P_GREEN
      "-d, --debug\n"
      "\tEnable Debug output\n"
      P_NORMAL
      "-v, --vendor\n"
      "\tSpecify an alternate vendor ID (in hex) to use (Default: %04X)\n"
      "-p, --product\n"
      "\tSpecify an alternate product ID (in hex) to use (Default: %04X)\n"
      P_BLUE
      "-l, --leds\n"
      "\tleds test (Breathing)\n"
      "-b, --buttons\n"
      "\tbuttons test (Button Interrupt)\n"
      "-m, --memory\n"
      "\tMemory Test\n"
      "-r, --dma_read\n"
      "\tDMA Read Demo\n"
      "-w, --dma_write\n"
      "\tDMA Write Demo\n"
      "--lcd\n"
      "\tLCD Color Test\n"
      P_NORMAL
      ,
      PROGRAM_NAME, DIONYSUS_VID, DIONYSUS_PID);
  exit(exit_status);
}

static void parse_args(struct arguments* args, int argc, char *const argv[]){
  int print_usage = 0;
  const char shortopts[] = "hdv:p:lbmrwe";
  struct option longopts[11];
  longopts[0].name = "help";
  longopts[0].has_arg = no_argument;
  longopts[0].flag = NULL;
  longopts[0].val = 'h';

  longopts[1].name = "debug";
  longopts[1].has_arg = no_argument;
  longopts[1].flag = NULL;
  longopts[1].val = 'd';

  longopts[2].name = "vendor";
  longopts[2].has_arg = required_argument;
  longopts[2].flag = NULL;
  longopts[2].val = 'v';

  longopts[3].name = "help";
  longopts[3].has_arg = required_argument;
  longopts[3].flag = NULL;
  longopts[3].val = 'p';

  longopts[4].name = "leds";
  longopts[4].has_arg = no_argument;
  longopts[4].flag = NULL;
  longopts[4].val = 'l';

  longopts[5].name = "buttons";
  longopts[5].has_arg = no_argument;
  longopts[5].flag = NULL;
  longopts[5].val = 'b';

  longopts[6].name = "memory";
  longopts[6].has_arg = no_argument;
  longopts[6].flag = NULL;
  longopts[6].val = 'm';

  longopts[7].name = "dma_read";
  longopts[7].has_arg = no_argument;
  longopts[7].flag = NULL;
  longopts[7].val = 'r';

  longopts[8].name = "dma_write";
  longopts[8].has_arg = no_argument;
  longopts[8].flag = NULL;
  longopts[8].val = 'w';

  longopts[9].name = "lcd";
  longopts[9].has_arg = no_argument;
  longopts[9].flag = NULL;
  longopts[9].val = 'e';

  longopts[10].name = 0;
  longopts[10].has_arg = 0;
  longopts[10].flag = 0;
  longopts[10].val = 0;

  while (1) {
    int option_idx = 0;
    int c = getopt_long(argc, argv, shortopts, longopts, &option_idx);
    if (c == -1){
      break;
    }
    switch (c){
      case 0:
          /* no arguments */
        if (print_usage){
          usage(EXIT_SUCCESS);
        }
        break;
      case 'h':
        usage(EXIT_SUCCESS);
        break;
      case 'd':
        printf ("debug enabled\n");
        args->debug = true;
        break;
      case 'v':
        args->vendor = strtol(optarg, (char**)0, 16);
        break;
      case 'p':
        args->product = strtol(optarg, (char**)0, 16);
        break;
      case 'l':
        args->leds = true;
        break;
      case 'b':
        args->buttons = true;
        break;
      case 'm':
        args->memory = true;
        break;
      case 'r':
        args->dma_read = true;
        break;
      case 'w':
        args->dma_write = true;
        break;
      case 'e':
        args->lcd_test = true;
      break;
      case '?': /* Fall through */
      default:
        printf ("Unknown Command\n");
        usage (EXIT_FAILURE);
        break;
    }
  }
}

void test_dma_reader(DMA_DEMO_READER * r){
  struct timeval start;
  struct timeval end;
  double interval = 1.0;
  double rate = 0;
  uint32_t count = 500;
  uint8_t * buffer = new uint8_t [r->get_buffer_size()];
  r->reset_dma_reader();
  r->enable_dma_reader(true);
  gettimeofday(&start, NULL);
  for (int i = 0; i < count; i++){
    r->dma_read(buffer);
    /*
    printf ("Buffer:\n");
    for (int i = 0; i < 16; i++){
      printf ("0x%02X ", buffer[i]);
    }
    printf ("\n");
    */

  }
  gettimeofday(&end, NULL);
  interval = TimevalDiff(&end, &start);
  printf ("Time difference: %f\n", interval);
  rate = (count * r->get_buffer_size()) / interval / 1e6;  //bytes/Sec
  printf ("Read Rate: %.2f MB/Sec\n", rate);

  r->enable_dma_reader(false);
  //for (int i = 0; i < r->get_buffer_size(); i++){
  delete(buffer);

}
void test_dma_writer(DMA_DEMO_WRITER * w){
  uint32_t count = 60;
  uint8_t * buffer = new uint8_t [w->get_buffer_size()];
  printf ("Testing DMA Writer\n");
  for (int i = 0; i < w->get_buffer_size(); i++){
    buffer[i] = (uint8_t) i;
  }
  w->reset_dma_writer();
  w->enable_dma_writer(true);
  for (int i = 0; i < count; i++){
    w->dma_write(buffer);
  }
  w->enable_dma_writer(false);
  delete(buffer);
}

void breath(Nysa *nysa, uint32_t device, uint32_t timeout, bool debug){
  struct timeval test_start;
  struct timeval test_now;
  double interval = 1.0;
  gettimeofday(&test_start, NULL);
  gettimeofday(&test_now, NULL);
  uint32_t period;
  uint32_t max_val = 4;
  uint32_t current = 0;
  uint32_t position = 0;
  uint32_t delay_count = 0;
  uint32_t delay = 4;
  uint8_t direction = 1;
  printf ("Breath\n");

  GPIO * gpio = new GPIO(nysa, device, debug);

  gpio->pinMode(0, OUTPUT); //LED 0
  gpio->pinMode(1, OUTPUT); //LED 1

  gpio->pinMode(2, INPUT); //Button 0
  gpio->pinMode(3, INPUT); //Button 1


  printf ("Max value: %d\n", max_val);

  while (interval < GPIO_TEST_WAIT){
    if (direction){
      if (current < max_val){
        if (position < max_val) {
          position++;
        }
        else {
          position = 0;
          if (delay_count < delay){
            delay_count++;
          }
          else {
            delay_count = 0;
            current++;
          }
        }
      }
      else {
        direction = 0;
      }
    }
    else {
      if (current > 0){
        if (position < max_val){
          position++;
        }
        else {
          position = 0;
          if (delay_count < delay){
            delay_count++;
          }
          else {
            delay_count = 0;
            current--;
          }
        }
      }
      else {
        //printf ("go up\n");
        direction = 1;
      }
    }

    if (position < current){
      gpio->set_gpios(0x00000002);
    }
    else {
      gpio->set_gpios(0x00000001);
    }
    gettimeofday(&test_now, NULL);
    interval = TimevalDiff(&test_now, &test_start);
  }
  gpio->digitalWrite(0, LOW);
  gpio->digitalWrite(1, LOW);
  delete(gpio);

};

void test_buttons(Nysa *nysa, uint32_t dev_index, bool debug){
  uint32_t interrupts = 0;
  if (dev_index == 0){
    printf ("Device index == 0!, this is the DRT!");
  }
  printf ("Setting up new gpio device\n");
  //Setup GPIO
  GPIO *gpio = new GPIO(nysa, dev_index, debug);
  printf ("Got new GPIO Device\n");
  gpio->pinMode(0, OUTPUT); //LED 0
  gpio->pinMode(1, OUTPUT); //LED 1

  gpio->pinMode(2, INPUT); //Button 0
  gpio->pinMode(3, INPUT); //Button 1

  gpio->set_interrupts_enable(0x0000000C);
  gpio->set_interrupts_edge_mask(0x0000000C);
  //gpio->set_interrupts_edge_mask(0x00000000);
  printf ("set pin modes!\n");

  //Pulse width of the LED

  printf ("Waiting for button press...\n");
  //wait 10 seconds
  printf ("Buttons: 0x%08X\n", gpio->get_gpios());
  gpio->get_interrupts();
  printf ("Interrupts: 0x%08X\n", gpio->get_interrupts());
  nysa->wait_for_interrupts(GPIO_TEST_WAIT * 1000, &interrupts);
  printf ("Interrupts: 0x%08X\n", interrupts);
  if (gpio->is_interrupt_for_device(interrupts) > 0){
    printf ("GPIO interrupts: 0x%08X\n", gpio->get_interrupts());
    printf ("Found interrupts for GPIOs: 0x%08X\n", gpio->get_gpios());
  }

  printf ("Set 0 to high\n");
  gpio->digitalWrite(0, LOW);
  gpio->digitalWrite(1, LOW);
  delete(gpio);
}


void test_lcd(Nysa *nysa, uint32_t dev_index, bool debug){
  uint8_t * buffer[COLOR_BUFFER_COUNT];
  uint32_t  red = 0x00FF0000;
  uint32_t  green = 0x0000FF00;
  uint32_t  blue = 0x000000FF;
  uint32_t  cyan = 0x0000FFF0;
  uint32_t  purple = 0x00F000FF;
  uint32_t  orange = 0x00FFF000;
  uint32_t  pink = 0x00FF7070;

  uint32_t size = 0;

  NH_LCD_480_272 * lcd = new NH_LCD_480_272(nysa, dev_index, debug);
  //Create the color buffers
  size = lcd->get_buffer_size();
  for (int i = 0; i < COLOR_BUFFER_COUNT; i++){
    buffer[i] = new uint8_t [size];
  }

  lcd->start();
  //Red
  for (int i = 0; i < size / 4; i += 4){
    buffer[0][i + 0] = (uint8_t) ((red >> 24) & 0xFF);
    buffer[0][i + 1] = (uint8_t) ((red >> 16) & 0xFF);
    buffer[0][i + 2] = (uint8_t) ((red >>  8) & 0xFF);
    buffer[0][i + 3] = (uint8_t) ((red      ) & 0xFF);
  }
  //Cyan
  for (int i = 0; i < size / 4; i += 4){
    buffer[1][i + 0] = (uint8_t) ((cyan >> 24) & 0xFF);
    buffer[1][i + 1] = (uint8_t) ((cyan >> 16) & 0xFF);
    buffer[1][i + 2] = (uint8_t) ((cyan >>  8) & 0xFF);
    buffer[1][i + 3] = (uint8_t) ((cyan      ) & 0xFF);
  }
  //Green
  for (int i = 0; i < size / 4; i += 4){
    buffer[2][i + 0] = (uint8_t) ((green >> 24) & 0xFF);
    buffer[2][i + 1] = (uint8_t) ((green >> 16) & 0xFF);
    buffer[2][i + 2] = (uint8_t) ((green >>  8) & 0xFF);
    buffer[2][i + 3] = (uint8_t) ((green      ) & 0xFF);
  }
  //Blue
  for (int i = 0; i < size / 4; i += 4){
    buffer[3][i + 0] = (uint8_t) ((purple >> 24) & 0xFF);
    buffer[3][i + 1] = (uint8_t) ((purple >> 16) & 0xFF);
    buffer[3][i + 2] = (uint8_t) ((purple >>  8) & 0xFF);
    buffer[3][i + 3] = (uint8_t) ((purple      ) & 0xFF);
  }

  //Write the LCD with the first color
  lcd->dma_write(buffer[0]);
  //Write the LCD with the second color
  lcd->dma_write(buffer[1]);
  //Write the LCD with the third color
  lcd->dma_write(buffer[2]);
  //Write the LCD with the fourth color
  lcd->dma_write(buffer[3]);

  lcd->stop();
  for (int i = 0; i < COLOR_BUFFER_COUNT; i++){
    delete(buffer[i]);
  }


  delete(lcd);
}

int main(int argc, char **argv){
  struct arguments args;
  args.vendor = DIONYSUS_VID;
  args.product = DIONYSUS_PID;
  args.debug = false;
  args.leds = false;
  args.buttons = false;
  args.memory = false;
  args.lcd_test = false;
  uint8_t* buffer = new uint8_t [8196];

  uint32_t num_devices;
  uint32_t device_type = 0;
  uint32_t device_addres = 0;
  uint32_t device_size = 0;
  bool memory_device = false;
  bool fail = false;
  uint32_t fail_count = 0;
  struct timeval start;
  struct timeval end;
  double interval = 1.0;
  double mem_size;
  double rate = 0;
  uint32_t memory_device_index = 0;

  parse_args(&args, argc, argv);

  //Dionysus dionysus = Dionysus((uint32_t)DEFAULT_BUFFER_SIZE, args.debug);
  Dionysus dionysus = Dionysus(args.debug);

  dionysus.open(args.vendor, args.product);
  if (dionysus.is_open()){
    dionysus.reset();
    //dionysus.program_fpga();
    //dionysus.soft_reset();
    gettimeofday(&start, NULL);
    dionysus.ping();
    gettimeofday(&end, NULL);
    interval = TimevalDiff(&end, &start);
    rate = 9 / interval;  //Megabytes/Sec
    printf ("Time difference: %f\n", interval);
    printf ("Ping Rate: %.2f Bytes/Sec\n", rate);


    printf ("Reading from the DRT\n");
    //dionysus.read_periph_data(0, 0, buffer, 32);
    dionysus.read_drt();
    for (int i = 1; i < dionysus.get_drt_device_count() + 1; i++){
      printf ("Device %d:\n", i);
      printf ("\tType:\t\t0x%04X\n", dionysus.get_drt_device_type(i));
      printf ("\tSub Type:\t0x%04X\n", dionysus.get_drt_device_sub_type(i));
      printf ("\tUser ID:\t0x%04X\n", dionysus.get_drt_device_user_id(i));
      printf ("\tSize:\t\t0x%08X (32-bit values)\n", dionysus.get_drt_device_size(i));
      printf ("\tAddress:\t0x%08X\n", dionysus.get_drt_device_addr(i));
      if (dionysus.is_memory_device(i)){
        printf ("\t\tOn the memory bus\n");
      }
    }
    //Buffer Data:
    //for (int i = 0; i < 32; i++){
    //  printf ("%02X ", buffer[i]);
    //}
    printf ("\n");
    //dionysus.read_periph_data(0, 0, buffer, 4096);
    printf ("Peripheral Write Test. read a lot of data from the DRT\n");
    printf ("\t(DRT will just ignore this data)\n");
    dionysus.write_periph_data(0, 0, buffer, 8192);

    printf ("Peripheral Read Test. Read a lot of data from the DRT\n");
    printf ("\t(Data from the DRT will just loop)\n");
    dionysus.read_periph_data(0, 0, buffer, 8192);
    delete(buffer);

    printf ("Look for a memory device\n");

    if (args.memory){
      for (int i = 1; i < dionysus.get_drt_device_count() + 1; i++){
        if (dionysus.get_drt_device_type(i) == 5){
          memory_device_index = i;
        }
      }
    }

    if (memory_device_index > 0){
      printf ("Found a memory device at position: %d\n", memory_device_index);

      printf ("Memory Test! Read and write: 0x%08X Bytes\n", dionysus.get_drt_device_size(memory_device_index));

      buffer = new uint8_t[dionysus.get_drt_device_size(memory_device_index)];
      for (int i = 0; i < dionysus.get_drt_device_size(memory_device_index); i++){
        buffer[i] = i;
      }
      printf ("Testing Full Memory Write Time...\n");
      gettimeofday(&start, NULL);
      dionysus.write_memory(0x00000000, buffer, dionysus.get_drt_device_size(memory_device_index));
      gettimeofday(&end, NULL);
      interval = TimevalDiff(&end, &start);
      mem_size = dionysus.get_drt_device_size(memory_device_index);
      rate = mem_size/ interval / 1e6;  //Megabytes/Sec
      printf ("Time difference: %f\n", interval);
      printf ("Write Rate: %.2f MB/Sec\n", rate);


      for (int i = 0; i < dionysus.get_drt_device_size(memory_device_index); i++){
        buffer[i] = 0;
      }
      gettimeofday(&start, NULL);
      dionysus.read_memory(0x00000000, buffer, dionysus.get_drt_device_size(memory_device_index));
      gettimeofday(&end, NULL);
      interval = TimevalDiff(&end, &start);
      mem_size = dionysus.get_drt_device_size(memory_device_index);
      rate = mem_size/ interval / 1e6;  //Megabytes/Sec
      printf ("Time difference: %f\n", interval);
      printf ("Read Rate: %.2f MB/Sec\n", rate);


      for (int i = 0; i < dionysus.get_drt_device_size(memory_device_index); i++){
        if (buffer[i] != i % 256){
          if (!fail){
            if (fail_count > 16){
              fail = true;
            }
            fail_count += 1;
            printf ("Failed @ 0x%08X\n", i);
            printf ("Value should be: 0x%08X but is: 0x%08X\n", i, buffer[i]);
          }
        }
      }
      if (!fail){
        printf ("Memory Test Passed!\n");
      }

      delete (buffer);
    }

    uint32_t device = 0;

    device = dionysus.find_device(get_gpio_device_type());

    if (device > 0){
      //Buttons and Interrupt test
      if (args.buttons){
        printf ("Testing buttons...\n");
        test_buttons(&dionysus, device, args.debug);
      }
      //LED Breath test
      if (args.leds){
        printf ("Testing LEDs...\n");
        breath(&dionysus, device, GPIO_TEST_WAIT, args.debug);
      }
    }

    //DMA Read Test
    device = dionysus.find_device(get_dma_reader_device_type());
    if (args.dma_read && (device > 0)){
      DMA_DEMO_READER dma_reader = DMA_DEMO_READER(&dionysus, device, args.debug);
      test_dma_reader(&dma_reader);
    }
    //DMA Write Test
    device = dionysus.find_device(get_dma_writer_device_type());
    printf ("Device: 0x%02X\n", device);
    if (args.dma_write && (device > 0)){
      DMA_DEMO_WRITER dma_writer = DMA_DEMO_WRITER(&dionysus, device, args.debug);
      test_dma_writer(&dma_writer);
    }

    //LCD Color Test
    device = dionysus.find_device(get_nh_lcd_480_272_type());
    if (device > 0){
      test_lcd(&dionysus, device, args.debug);
    }
    dionysus.close();
  }
  return 0;
}



