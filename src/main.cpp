#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include "ftdi.hpp"
#include "dionysus.hpp"
#include "gpio.hpp"
#include "arduino.hpp"

#define PROGRAM_NAME "dionysus-ftdi"


#define MEMORY_TEST false
//5 seconds
#define GPIO_TEST_WAIT 10

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
};

static double TimevalDiff(const struct timeval *a, const struct timeval *b)
{
    return (a->tv_sec - b->tv_sec) + 1e-6 * (a->tv_usec - b->tv_usec);
}

static void usage (int exit_status){
  fprintf (exit_status == EXIT_SUCCESS ? stdout : stderr,
      "\n"
      "USAGE: %s [-v <vendor>] [-p <product>] [-d]\n"
      "\n"
      "Options:\n"
      " -h, --help\n"
      "\tPrints this helpful message\n"
      " -d, --debug\n"
      "\tEnable Debug output\n"
      " -v, --vendor\n"
      "\tSpecify an alternate vendor ID (in hex) to use (Default: %04X)\n"
      " -p, --product\n"
      "\tSpecify an alternate product ID (in hex) to use (Default: %04X)\n"
      ,
      PROGRAM_NAME, DIONYSUS_VID, DIONYSUS_PID);
  exit(exit_status);
}

static void parse_args(struct arguments* args, int argc, char *const argv[]){
  int print_usage = 0;
  const char shortopts[] = "hdv:p:";
  struct option longopts[5];
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

  longopts[4].name = 0;
  longopts[4].has_arg = 0;
  longopts[4].flag = 0;
  longopts[4].val = 0;
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
      case '?': /* Fall through */
      default:
        printf ("Unknown Command\n");
        usage (EXIT_FAILURE);
        break;
    }
  }
}

void breath(GPIO *g, uint32_t timeout){
  struct timeval test_start;
  struct timeval test_now;
  double interval = 1.0;
  gettimeofday(&test_start, NULL);
  gettimeofday(&test_now, NULL);
  uint32_t period;
  uint32_t max_val = 10;
  uint32_t current = 0;
  uint32_t position = 0;
  uint32_t delay_count = 0;
  uint32_t delay = 0;
  uint8_t direction = 1;
  printf ("Breath\n");

  printf ("Max value: %d\n", max_val);

  while (interval < GPIO_TEST_WAIT){
    if (direction){
      if (current < max_val){
        if (position < max_val) {
          //printf ("pos++\n");
          position++;
        }
        else {
          //printf ("position = 0\n");
          position = 0;
          if (delay_count < delay){
            delay_count++;
          }
          else {
            //printf ("increment count\n");
            delay_count = 0;
            current++;
          }
        }
      }
      else {
        //printf ("go down\n");
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
      //printf ("HI\n");
      //g->digitalWrite(0, LOW);
      g->set_gpios(0x00000002);
    }
    else {
      //printf ("LOW\n");
      g->set_gpios(0x00000001);
      //g->digitalWrite(0, HIGH);
    }
    gettimeofday(&test_now, NULL);
    interval = TimevalDiff(&test_now, &test_start);
  }
};

void test_gpios(Nysa *nysa, uint32_t dev_index, bool debug){
  if (dev_index == 0){
    printf ("Device index == 0!, this is the DRT!");
  }
  struct timeval test_start;
  struct timeval test_now;
  double interval = 1.0;

  double led_interval = 1.0;
  printf ("Setting up new gpio device\n");
  //Setup GPIO
  GPIO *gpio = new GPIO(nysa, dev_index, debug);
  printf ("Got new GPIO Device\n");
  try {
    gpio->pinMode(0, OUTPUT); //LED 0
    gpio->pinMode(1, OUTPUT); //LED 1

    gpio->pinMode(2, INPUT); //Button 0
    gpio->pinMode(3, INPUT); //Button 1
  }
  catch (int e) {
    printf ("Error while setting pinMode: Error: %d\n", e);
  }
  printf ("set pin modes!\n");

  //Length of GPIO tests
  gettimeofday(&test_start, NULL);
  gettimeofday(&test_now, NULL);

  //Pulse width of the LED

  interval = TimevalDiff(&test_now, &test_start);
  printf ("Interval: %f\n", interval);
  breath(gpio, GPIO_TEST_WAIT);

  /*
  while (interval < GPIO_TEST_WAIT){

    gpio->toggle(0);
    usleep(1000000);

    gettimeofday(&test_now, NULL);
    interval = TimevalDiff(&test_now, &test_start);
  //printf ("Interval: %f\n", interval);
  }
  */
  //Loop for about two seconds
  printf ("Set 0 to high\n");
  gpio->digitalWrite(0, LOW);
  gpio->digitalWrite(1, LOW);
}

int main(int argc, char **argv){
  struct arguments args;
  args.vendor = DIONYSUS_VID;
  args.product = DIONYSUS_PID;
  args.debug = false;
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
      printf ("\tType:\t\t0x%08X\n", dionysus.get_drt_device_type(i));
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

    if (MEMORY_TEST){
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
    uint32_t gpio_device = 0;
    for (int i = 1; i < dionysus.get_drt_device_count() + 1; i++){
      if (dionysus.get_drt_device_type(i) == get_gpio_device_type()){
        gpio_device = i;
      }
    }
    if (gpio_device > 0){
      test_gpios(&dionysus, gpio_device, args.debug);
    }


    dionysus.close();
  }
  return 0;
}


