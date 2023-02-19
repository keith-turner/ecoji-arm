#include <stdint.h>
#include <stdio.h>

#include "mapping.h"

int append(char *output, int idx, uint64_t emoji) {
  int len = emoji >> 56;

  if (len == 3) {
    output[idx++] = 0xff & (emoji >> 16);
    output[idx++] = 0xff & (emoji >> 8);
    output[idx++] = 0xff & (emoji);
  } else {
    output[idx++] = 0xff & (emoji >> 24);
    output[idx++] = 0xff & (emoji >> 16);
    output[idx++] = 0xff & (emoji >> 8);
    output[idx++] = 0xff & (emoji);
  }

  return idx;
}

int ecoji_encode(const uint8_t *input, int input_len, char *output) {
  int oidx = 0;

  int whole5_len = (input_len / 5) * 5;

  uint64_t bits;

  for (int i = 0; i < whole5_len; i += 5) {
    bits = ((uint64_t)input[i]) << 32 | ((uint64_t)input[i + 1]) << 24 |
           input[i + 2] << 16 | input[i + 3] << 8 | input[i + 4];
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 30)]);
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, emojisV2[0x03ff & bits]);
  }

  int left = input_len - whole5_len;

  int i = whole5_len;

  switch (left) {
  case 1:
    oidx = append(output, oidx, emojisV2[input[i] << 2]);
    oidx = append(output, oidx, padding);
    break;
  case 2:
    bits = input[i] << 12 | input[i + 1] << 4;
    oidx = append(output, oidx, emojisV2[bits >> 10]);
    oidx = append(output, oidx, emojisV2[0x03ff & bits]);
    oidx = append(output, oidx, padding);
    break;
  case 3:
    bits = input[i] << 22 | input[i + 1] << 14 | input[i + 2] << 6;
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, emojisV2[0x03ff & bits]);
    oidx = append(output, oidx, padding);
    break;
  case 4:
    bits = ((uint64_t)input[i]) << 32 | ((uint64_t)input[i + 1]) << 24 |
           input[i + 2] << 16 | input[i + 3] << 8;
    oidx = append(output, oidx, emojisV2[bits >> 30]);
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojisV2[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, paddingLastV2[0x03 & (bits >> 8)]);
    break;
  }

  return oidx;
}
