# ============================================================================
# Build Configuration
# ============================================================================
# Edit this file to customize your build without touching the Makefiles

# Build Configuration
CONFIG ?= debug
ARCH ?= x86_64

# Image Configuration
IMAGE_TYPE ?= disk
IMAGE_FS ?= fat32
IMAGE_SIZE ?= 250M

# Toolchain Configuration
TOOLCHAIN_DIR ?= toolchain
BINUTILS_VERSION := 2.44
GCC_VERSION := 15.1.0

# Toolchain prefix based on architecture
ifeq ($(ARCH),x86_64)
    TOOLCHAIN_PREFIX := x86_64-elf
    AS_FORMAT := elf64
    ARCH_FLAGS := -mno-red-zone
else ifeq ($(ARCH),i686)
    TOOLCHAIN_PREFIX := i686-elf
    AS_FORMAT := elf
    ARCH_FLAGS :=
else
    $(error Unsupported architecture: $(ARCH))
endif

# Derived paths
TOOLCHAIN_PATH := $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)
TOOLCHAIN_BIN := $(TOOLCHAIN_PATH)/bin
TOOLCHAIN_LIBGCC := $(TOOLCHAIN_PATH)/lib/gcc/$(TOOLCHAIN_PREFIX)/$(GCC_VERSION)

# Cross-compiler tools
AR := $(TOOLCHAIN_PREFIX)-ar
AS := nasm
CC := $(TOOLCHAIN_PREFIX)-gcc
CXX := $(TOOLCHAIN_PREFIX)-g++
LD := $(TOOLCHAIN_PREFIX)-g++
OBJCOPY := $(TOOLCHAIN_PREFIX)-objcopy
RANLIB := $(TOOLCHAIN_PREFIX)-ranlib
STRIP := $(TOOLCHAIN_PREFIX)-strip

# Common flags
ASFLAGS := -f $(AS_FORMAT) -g
CFLAGS := -std=c11 -ffreestanding -nostdlib -fno-stack-protector $(ARCH_FLAGS) -g
CXXFLAGS := -std=c++17 -ffreestanding -nostdlib -fno-stack-protector -fno-exceptions -fno-rtti $(ARCH_FLAGS) -g
LDFLAGS := -nostdlib

# Optimization based on config
ifeq ($(CONFIG),release)
    CFLAGS += -O3
    CXXFLAGS += -O3
else
    CFLAGS += -O0
    CXXFLAGS += -O0
endif

# UEFI build paths (system dependencies)
UEFI_INCLUDE := /usr/include/efi
UEFI_LIB := /usr/lib
UEFI_CRT0 := $(UEFI_LIB)/crt0-efi-x86_64.o
UEFI_LDS := $(UEFI_LIB)/elf_x86_64_efi.lds

# Build directories
BUILD_DIR := build/$(ARCH)_$(CONFIG)
BUILD_UEFI := $(BUILD_DIR)/uefi_boot
BUILD_KERNEL := $(BUILD_DIR)/kernel
BUILD_CORE := $(BUILD_DIR)/libcore
BUILD_IMAGE := $(BUILD_DIR)/image

# Export for sub-makefiles
export BUILD_CORE

# Output files
BOOTLOADER_EFI := $(BUILD_UEFI)/BOOTX64.EFI
KERNEL_BIN := $(BUILD_KERNEL)/kernel.bin
IMAGE_FILE := $(BUILD_IMAGE)/os_image.img

# Export PATH for toolchain
export PATH := $(TOOLCHAIN_BIN):$(PATH)