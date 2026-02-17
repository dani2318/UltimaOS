    set disassembly-flavor intel
    target remote | qemu-system-i386 -S -gdb stdio -m 32 -hda /home/dani23/NeoOS/build/i686_debug/image.img
    symbol-file /home/dani23/NeoOS/build/i686_debug/kernel/kernel.elf
