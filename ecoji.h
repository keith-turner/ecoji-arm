#ifndef ECOJI_API_H
#define ECOJI_API_H

int ecoji_encode_v1(const char *input, int input_len,  const char *output);

int ecoji_encode_v2(const char *input, int input_len,  const char *output);

int ecoji_decode(const char *input, int input_len,  const char *output, char *uncomsumed, int *uclen);

#endif //ECOJI_API_H
