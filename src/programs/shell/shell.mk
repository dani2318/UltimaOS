# src/programs/shell/shell.mk

# Use the absolute path you found
CC = /home/dani23/NeoOS/toolchain/x86_64-elf/bin/x86_64-elf-gcc
LD = /home/dani23/NeoOS/toolchain/x86_64-elf/bin/x86_64-elf-ld

CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I../../kernel
LDFLAGS = -T linker.ld -static -nostdlib

OUTPUT = ../../../build/x86_64_debug/programs/shell.elf

all: $(OUTPUT)

$(OUTPUT): shell.o
	@mkdir -p ../../../build/x86_64_debug/programs
	$(LD) $(LDFLAGS) -o $(OUTPUT) shell.o

shell.o: shell.cpp
	$(CC) $(CFLAGS) -c shell.cpp -o shell.o