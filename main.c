#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ecoji.h"

int readFully(char *buf, int len) {
  int num_read = read(STDIN_FILENO, buf, len);
  if (num_read <= 0) {
    return num_read;
  }
  while (num_read < len) {
    int tmp = read(STDIN_FILENO, buf + num_read, len - num_read);
    if (tmp < 0) {
      return tmp;
    }
    if (tmp == 0) {
      break;
    }
    num_read += tmp;
  }

  return num_read;
}

int writeFully(char *buf, int len) {
  int num_wrote = write(STDOUT_FILENO, buf, len);
  if (num_wrote <= 0) {
    return num_wrote;
  }

  while (num_wrote < len) {
    int tmp = write(STDOUT_FILENO, buf + num_wrote, len - num_wrote);
    if (tmp < 0) {
      return tmp;
    }

    if (tmp == 0) {
      break;
    }

    num_wrote += tmp;
  }

  return num_wrote;
}

int encode() {
  char input[100000];
  char output[400000];

  int num_read;
  while ((num_read = readFully(input, 100000)) > 0) {
    int output_len = ecoji_encode_v2(input, num_read, output);
    writeFully(output, output_len);
  }

  return 0;
}

int decode() {
  char input[400000];
  char output[100000];
  char uncomsumed[4];
  int uclen = 0;

  int num_read;
  while ((num_read = readFully(input + uclen, 400000 - uclen)) > 0) {

    // fprintf(stderr, "num_read %d %d\n", num_read, uclen);

    int output_len =
        ecoji_decode(input, num_read + uclen, output, uncomsumed, &uclen);
    if (output_len == -1) {
      return -1;
    }
    writeFully(output, output_len);

    // fprintf(stderr, "unclen %d\n", uclen);

    for (int i = 0; i < uclen; i++) {
      input[i] = uncomsumed[i];
    }
  }

  // TODO handle uclen > 0

  return 0;
}

int main(int argc, char *argv[]) {
  int opt;

  if ((opt = getopt(argc, argv, "ed")) != -1) {
    switch (opt) {
    case 'e':
      encode();
      break;
    case 'd':
      exit(decode());
      break;
    default:
      fprintf(stderr, "Usage: %s [-ed]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}
