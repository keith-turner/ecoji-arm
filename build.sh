#!/bin/bash

go run gen_c.go | clang-format > mapping.h
clang-format -i *.c
gcc -O3 *.c -o ecoji_c
