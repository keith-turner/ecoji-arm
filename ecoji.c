#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>

#include "ecoji_generated.h"

int append(char *output, int idx, uint64_t emoji) {
  int len = emoji >> 56;

  if (len == 3) {
    output[idx++] = 0xff & (emoji >> 16);
    output[idx++] = 0xff & (emoji >> 8);
    output[idx++] = 0xff & (emoji);
    return idx;
  } else {
    *((uint32_t *)(&output[idx])) = htonl(emoji);
    return idx + 4;
  }
}

int ecoji_encode(const int64_t *emojis, const int64_t *paddingLast,
                 const uint8_t *input, int input_len, char *output) {
  int oidx = 0;

  int whole5_len = (input_len / 5) * 5;

  uint64_t bits;

  for (int i = 0; i < whole5_len; i += 5) {
    bits = ((uint64_t)input[i]) << 32 | ntohl(*((uint32_t *)(&input[i + 1])));
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 30)]);
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, emojis[0x03ff & bits]);
  }

  int left = input_len - whole5_len;

  int i = whole5_len;

  switch (left) {
  case 1:
    oidx = append(output, oidx, emojis[input[i] << 2]);
    oidx = append(output, oidx, padding);
    break;
  case 2:
    bits = input[i] << 12 | input[i + 1] << 4;
    oidx = append(output, oidx, emojis[bits >> 10]);
    oidx = append(output, oidx, emojis[0x03ff & bits]);
    oidx = append(output, oidx, padding);
    break;
  case 3:
    bits = input[i] << 22 | input[i + 1] << 14 | input[i + 2] << 6;
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, emojis[0x03ff & bits]);
    oidx = append(output, oidx, padding);
    break;
  case 4:
    bits = ((uint64_t)input[i]) << 32 | ((uint64_t)input[i + 1]) << 24 |
           input[i + 2] << 16 | input[i + 3] << 8;
    oidx = append(output, oidx, emojis[bits >> 30]);
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 20)]);
    oidx = append(output, oidx, emojis[0x03ff & (bits >> 10)]);
    oidx = append(output, oidx, paddingLast[0x03 & (bits >> 8)]);
    break;
  }

  return oidx;
}

int ecoji_encode_v1(const uint8_t *input, int input_len, char *output) {
  return ecoji_encode(emojisV1, paddingLastV1, input, input_len, output);
}

int ecoji_encode_v2(const uint8_t *input, int input_len, char *output) {
  return ecoji_encode(emojisV2, paddingLastV2, input, input_len, output);
}

int isPadding(int64_t d) { return (0x0f & (d >> 12)) == 1; }

int isLastPadding(int64_t d) { return (0x0f & (d >> 12)) == 2; }

int ecoji_decode(FILE *infp, FILE *outfp) {

  while (1) {

    int sawErr = 0;
    int32_t d1 = ecoji_decode_emoji(infp);
    if (d1 < 0) {
      if (feof(infp) == 0) {
        return -1;
      } else {
        return 0;
      }
    }

    // TODO check for padding

    int sawPadding = 0;

    int32_t d2 = ecoji_decode_emoji(infp);
    if (d2 < 0) {
      if (d2 == -1) {
        return -1;
      }

      sawPadding = 1;
    }

    int32_t d3 = ecoji_decode_emoji(infp);
    if (d3 < 0) {
      if (d3 == -1 && feof(infp) == 0) {
        return -1;
      }

      sawPadding = 1;
    }

    int32_t d4 = ecoji_decode_emoji(infp);
    if (d4 < 0) {
      if (d4 == -1 && feof(infp) == 0) {
        return -1;
      }

      sawPadding = 1;
    }

    int len = 5;

    if (sawPadding) {
      if (isPadding(d2)) {
        len = 1;
      } else if (isPadding(d3)) {
        len = 2;
      } else if (isPadding(d4)) {
        len = 3;
      } else if (isLastPadding(d4)) {
        len = 4;
      }
    }

#ifdef ARM_ASM
    uint64_t bits;
    unsigned char data[8];
    __asm("UBFIZ %[bits],%[d1],30,10\n\t"
          "BFI %[bits],%[d2],20,10\n\t"
          "BFI %[bits],%[d3],10,10\n\t"
          "BFI %[bits],%[d4],0,10\n\t"
          "REV %[bits],%[bits]\n\t"
          "STR %[bits],%[data]"
          : [bits] "=r"(bits), [data] "=m"(data)
          : [d1] "r"(d1), [d2] "r"(d2), [d3] "r"(d3), [d4] "r"(d4));

    unsigned char *output = data + 3;

#else
    uint64_t bits = (0x3ff & (uint64_t)d1) << 30 |
                    (0x3ff & (uint64_t)d2) << 20 |
                    (0x3ff & (uint64_t)d3) << 10 | 0x3ff & (uint64_t)d4;

    unsigned char output[5];
    output[0] = 0xff & (bits >> 32);
    *((uint32_t *)(&output[1])) = ntohl(bits);

#endif

    if (fwrite_unlocked(output, 1, len, outfp) != len) {
      return -1;
    }
  }
}
