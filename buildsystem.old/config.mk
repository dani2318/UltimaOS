
DISK_SIZE_MAKEFILE = 16777216

export CFLAGS = -std=c99 -g
export ASMFLAGS =
export CC = gcc
export CXX = g++
export LD = gcc
export ASM = nasm
export LINKFLAGS =
export LIBS =
current_dir = $(shell pwd)
export TARGET = i686-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CFLAGS = -std=c99 -g #-O2
export TARGET_CC = $(current_dir)/toolchain/i686-elf/bin/i686-elf-gcc
export TARGET_CXX = $(current_dir)/toolchain/i686-elf/bin/i686-elf-g++
export TARGET_LD = $(current_dir)/toolchain/i686-elf/bin/i686-elf-gcc
export TARGET_LINKFLAGS =
export TARGET_LIBS =

export BUILD_DIR = $(abspath build)
export SRC_DIR = $(abspath src)

