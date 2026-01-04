#!/usr/bin/sh

mkdir -p logs/
date=$(date '+%d-%m-%Y--%H.%M.%S')
logfile="logs/qemu-logs-$date.log"
# touch "$logfile" # Not strictly needed if using stdio

# 1. Locate OVMF (UEFI Firmware)
# On WSL Ubuntu, this is the standard path after 'sudo apt install ovmf'
OVMF_PATH="/usr/share/ovmf/OVMF.fd"

if [ ! -f "$OVMF_PATH" ]; then
    echo "Error: OVMF firmware not found at $OVMF_PATH"
    echo "Please run: sudo apt install ovmf"
    exit 3
fi

# 2. Base QEMU Arguments
# Note: x86_64 is required for UEFI BOOTX64.EFI
QEMU="qemu-system-x86_64"
QEMU_ARGS="-cpu qemu64 -m 4G -serial stdio -d int,guest_errors -D qemu.log -bios $OVMF_PATH"

if [ "$#" -le 1 ]; then
    echo "Usage: ./run.sh <image_type> <image>"
    exit 1
fi

case "$1" in
    "floppy")
        # UEFI rarely boots from legacy floppy; usually treated as a small disk
        QEMU_ARGS="${QEMU_ARGS} -drive file=$2,format=raw,if=floppy"
    ;;
    "disk")
        # Standard drive mapping
        QEMU_ARGS="${QEMU_ARGS} -drive file=$2,format=raw"
    ;;
    *)
        echo "Unknown image type $1."
        exit 2
esac

# 4. Run QEMU
# If you are in WSL without a GUI, add -nographic or -display curses
echo "Launching QEMU..."
$QEMU $QEMU_ARGS