#include <arpa/inet.h>
#include <stdio.h>

#include "ecoji_generated.h"

typedef struct {
  char *output;
  int _wrap;
  int emojis_output;
  int idx;
  int ecoji_vers;
  int wrap;
} ecoji_encode_state;

void append(ecoji_encode_state *state, uint64_t emoji) {
  int len = emoji >> 56;

  if (len == 3) {
    state->output[state->idx++] = 0xff & (emoji >> 16);
    state->output[state->idx++] = 0xff & (emoji >> 8);
    state->output[state->idx++] = 0xff & (emoji);
    if (state->_wrap > 0) {
      state->emojis_output++;
      if (state->emojis_output >= state->_wrap) {
        state->output[state->idx++] = '\n';
        state->emojis_output = 0;
      }
    }
  } else {
    *((uint32_t *)(&(state->output[state->idx]))) = htonl(emoji);
    if (state->_wrap > 0) {
      state->emojis_output++;
      if (state->emojis_output >= state->_wrap) {
        state->output[state->idx + 4] = '\n';
        state->emojis_output = 0;
        state->idx += 5;
        return;
      }
    }
    state->idx += 4;
  }
}

int ecoji_encode(const int64_t *emojis, const int64_t *paddingLast,
                 const uint8_t *input, int input_len, char *output,
                 ecoji_encode_state *state) {

  int whole5_len = (input_len / 5) * 5;

  uint64_t bits;

  for (int i = 0; i < whole5_len; i += 5) {
    bits = ((uint64_t)input[i]) << 32 | ntohl(*((uint32_t *)(&input[i + 1])));
    append(state, emojis[0x03ff & (bits >> 30)]);
    append(state, emojis[0x03ff & (bits >> 20)]);
    append(state, emojis[0x03ff & (bits >> 10)]);
    append(state, emojis[0x03ff & bits]);
  }

  int left = input_len - whole5_len;

  int i = whole5_len;

  switch (left) {
  case 1:
    append(state, emojis[input[i] << 2]);
    append(state, padding);
    if (state->ecoji_vers == 1) {
      append(state, padding);
      append(state, padding);
    }
    break;
  case 2:
    bits = input[i] << 12 | input[i + 1] << 4;
    append(state, emojis[bits >> 10]);
    append(state, emojis[0x03ff & bits]);
    append(state, padding);
    if (state->ecoji_vers == 1) {
      append(state, padding);
    }
    break;
  case 3:
    bits = input[i] << 22 | input[i + 1] << 14 | input[i + 2] << 6;
    append(state, emojis[0x03ff & (bits >> 20)]);
    append(state, emojis[0x03ff & (bits >> 10)]);
    append(state, emojis[0x03ff & bits]);
    append(state, padding);
    break;
  case 4:
    bits = ((uint64_t)input[i]) << 32 | ((uint64_t)input[i + 1]) << 24 |
           input[i + 2] << 16 | input[i + 3] << 8;
    append(state, emojis[bits >> 30]);
    append(state, emojis[0x03ff & (bits >> 20)]);
    append(state, emojis[0x03ff & (bits >> 10)]);
    append(state, paddingLast[0x03 & (bits >> 8)]);
    break;
  }

  return state->idx;
}

int readFully(FILE *infp, char *buf, int len) {

  int num_read = fread(buf, 1, len, infp);

  if (num_read <= 0) {
    return num_read;
  }
  while (num_read < len) {
    int tmp = fread(buf + num_read, 1, len - num_read, infp);
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

int writeFully(FILE *outfp, char *buf, int len) {
  int num_wrote = fwrite(buf, 1, len, outfp);
  if (num_wrote <= 0) {
    return num_wrote;
  }

  while (num_wrote < len) {
    int tmp = fwrite(buf + num_wrote, 1, len - num_wrote, outfp);
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

int encode_buffered(const int64_t *emojis, const int64_t *paddingLast,
                    FILE *infp, FILE *outfp, int ecoji_vers, int wrap) {

  char input[100000];
  char output[500000];

  ecoji_encode_state state;
  state.emojis_output = 0;
  state.output = output;
  state._wrap = wrap;
  state.idx = 0;
  state.ecoji_vers = ecoji_vers;

  int num_read;
  while ((num_read = readFully(infp, input, 100000)) > 0) {
    int output_len =
        ecoji_encode(emojis, paddingLast, input, num_read, output, &state);
    writeFully(outfp, output, output_len);
    state.idx = 0;
  }

  if (state._wrap > 0 && state.emojis_output > 0) {
    putc('\n', outfp);
  }

  return 0;
}

int ecoji_encode_v1(FILE *infp, FILE *outfp, int wrap) {
  return encode_buffered(emojisV1, paddingLastV1, infp, outfp, 1, wrap);
}

int ecoji_encode_v2(FILE *infp, FILE *outfp, int wrap) {
  return encode_buffered(emojisV2, paddingLastV2, infp, outfp, 2, wrap);
}

int isPadding(int64_t d) { return (0x0f & (d >> 12)) == 1; }

int isLastPadding(int64_t d) { return (0x0f & (d >> 12)) == 2; }

int32_t decode_and_check(FILE *infp, int32_t *expected_ver) {
  int32_t d = ecoji_decode_emoji(infp);
  if ((d & *expected_ver) != 0) {
    return d;
  } else {
    if (*expected_ver == ECOJI_VER_ONE_AND_TWO) {
      // this is the first time to see a version one only OR version two only
      // emoji.  So lets add that to our expected version
      *expected_ver = *expected_ver | (ECOJI_VER_MASK & d);
      return d;
    } else {
      // seeing an unexpected emoji version
      // TODO push back and concat may not work for ecoji 2 followed by ecoji 1
      // because we suppress the ecoji1 here after seeing 2?
      return -1;
    }
  }
}

int push_back(FILE *infp, int32_t d) {
  if (d == -1)
    return -1;

  int64_t emoji_info = emojisV2[0x3ff & d];

  unsigned char output[4];

  ecoji_encode_state state;
  state.output = output;
  state._wrap = 0;
  state.idx = 0;

  append(&state, emoji_info);

  for (int i = state.idx - 1; i >= 0; i--) {
    if (ungetc(output[i], infp) != output[i]) {
      fprintf(stderr, "ungetc failed \n");
      return -1;
    }
  }

  return 0;
}

int ecoji_decode(FILE *infp, FILE *outfp) {

  int32_t expected_ver = ECOJI_VER_ONE_AND_TWO;

  while (1) {

    int32_t d1 = decode_and_check(infp, &expected_ver);
    if (d1 < 0) {
      if (feof(infp) == 0) {
        return -1;
      } else {
        return 0;
      }
    }

    int checksNeeded = 0;

    int32_t d2 = decode_and_check(infp, &expected_ver);
    if (d2 < 0) {
      if (d2 == -1) {
        return -1;
      }

      checksNeeded = 1;
    }

    int32_t d3 = decode_and_check(infp, &expected_ver);
    if (d3 < 0) {
      if (d3 == -1 && feof(infp) == 0) {
        return -1;
      }

      checksNeeded = 1;
    }

    int32_t d4 = decode_and_check(infp, &expected_ver);
    if (d4 < 0) {
      if (d4 == -1 && feof(infp) == 0) {
        return -1;
      }

      checksNeeded = 1;
    }

    int len = 5;

    if (checksNeeded) {

      if (isLastPadding(d2) || isLastPadding(d3)) {
        return -1;
      }

      if (isPadding(d2)) {
        len = 1;
        if ((expected_ver & ECOJI_VER_ONE_ONLY) != 0 && !isPadding(d3) &&
            !isPadding(d4)) {
          // saw ecoji ver1 so would expect full padding
          return -1;
        }

        if ((expected_ver & ECOJI_VER_ONE_ONLY) == 0 && !isPadding(d3) &&
            !isPadding(d4)) {

          if (d4 != -1) {
            if (push_back(infp, d4) == -1) {
              return -1;
            }
          }
          if (d3 != -1) {
            if (push_back(infp, d3) == -1) {
              return -1;
            }
          }
        }

      } else if (isPadding(d3)) {
        len = 2;
        if ((expected_ver & ECOJI_VER_ONE_ONLY) != 0 && !isPadding(d4)) {
          // saw ecoji ver1 so would expect full padding
          return -1;
        }

        if ((expected_ver & ECOJI_VER_ONE_ONLY) == 0 && !isPadding(d4)) {
          if (d4 != -1) {
            if (push_back(infp, d4) == -1) {
              return -1;
            }
          }
        }
      } else if (isPadding(d4)) {
        len = 3;
      } else if (isLastPadding(d4)) {
        len = 4;
      } else {
        return -1;
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
