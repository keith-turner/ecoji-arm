#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ecoji.h"

int main(int argc, char *argv[]) {
  int wrap = 72;
  int enc = -1;
  int c;
  while ((c = getopt(argc, argv, "eEdw:")) != -1) {
    switch (c) {
    case 'e':
      enc = 2;
      break;
    case 'E':
      enc = 1;
      break;
    case 'd':
      exit(ecoji_decode(stdin, stdout));
      break;
    case 'w':
      errno = 0;
      char *endptr;
      wrap = strtol(optarg, &endptr, 10);
      if (errno != 0) {
        perror("strtol");
      }
      if (endptr == optarg || wrap < 0) {
        fprintf(stderr, "Wrap '%s' is not a non negative integer.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case '?':
      if (optopt == 'w')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    default:
      exit(EXIT_FAILURE);
    }
  }

  if (enc == 2) {
    ecoji_encode_v2(stdin, stdout, wrap);
  } else if (enc == 1) {
    ecoji_encode_v1(stdin, stdout, wrap);
  } else {
    exit(EXIT_FAILURE);
  }
}
