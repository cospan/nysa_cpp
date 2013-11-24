#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "ftdi.hpp"
#include "dionysus.hpp"

#define PROGRAM_NAME "dionysus-ftdi"

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

int main(int argc, char **argv){
  struct arguments args;
  args.vendor = DIONYSUS_VID;
  args.product = DIONYSUS_PID;
  args.debug = false;

  parse_args(&args, argc, argv);

  //Dionysus dionysus = Dionysus((uint32_t)DEFAULT_BUFFER_SIZE, args.debug);
  Dionysus dionysus = Dionysus(args.debug);

  dionysus.open(args.vendor, args.product);
  if (dionysus.is_open()){
    dionysus.reset();
    //dionysus.program_fpga();
    //dionysus.soft_reset();
    dionysus.ping();
    dionysus.close();
  }
  return 0;
}


