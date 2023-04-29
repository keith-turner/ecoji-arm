#!/bin/bash

go run gen_c.go | clang-format > ecoji_generated.h
clang-format -i *.c

if [[ $(uname -m) == *"aarch64"* ]]; then
	BUILD_ARGS="-DARM_ASM"
fi

gcc $BUILD_ARGS -O3 main.c ecoji.c -o ecoji_c
gcc $BUILD_ARGS -fverbose-asm -O3 -S ecoji.c
