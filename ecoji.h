#ifndef ECOJI_API_H
#define ECOJI_API_H

int ecoji_encode_v1(FILE* infp,  FILE *outfp, int wrap);

int ecoji_encode_v2(FILE* infp,  FILE *outfp, int wrap);

int ecoji_decode(FILE* infp,  FILE *outfp);

#endif //ECOJI_API_H
