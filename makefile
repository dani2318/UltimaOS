# ============================================================================
# Main Makefile - Custom OS Build System
# ============================================================================

# Include configuration
include config.mk

# Phony targets
.PHONY: all clean bootloader kernel libcore image run debug bochs toolchain help

# Default target
all: image 

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build complete OS image (default)"
	@echo "  bootloader - Build UEFI bootloader"
	@echo "  kernel     - Build kernel"
	@echo "  libcore    - Build core library"
	@echo "  image      - Create bootable disk image"
	@echo "  run        - Run in QEMU"
	@echo "  debug      - Run in QEMU with GDB"
	@echo "  bochs      - Run in Bochs"
	@echo "  toolchain  - Setup cross-compiler toolchain"
	@echo "  clean      - Remove all build artifacts"
	@echo ""
	@echo "Configuration options (edit config.mk or override on command line):"
	@echo "  CONFIG=$(CONFIG)      - Build configuration [debug|release]"
	@echo "  ARCH=$(ARCH)        - Target architecture [x86_64|i686]"
	@echo "  IMAGE_TYPE=$(IMAGE_TYPE)  - Image type [disk|floppy]"
	@echo "  IMAGE_FS=$(IMAGE_FS)    - Filesystem [fat32|fat16|fat12|ext2]"

# Build components
libcore:
	@echo "Building libcore..."
	@$(MAKE) -f src/libs/core/core.mk BUILD_DIR=$(BUILD_CORE)

bootloader: libcore
	@echo "Building UEFI bootloader..."
	@$(MAKE) -f src/boot/uefi_boot/uefi_boot.mk BUILD_DIR=$(BUILD_UEFI)

kernel: libcore
	@echo "Building kernel..."
	@$(MAKE) -f src/kernel/kernel.mk BUILD_DIR=$(BUILD_KERNEL) BUILD_CORE=$(BUILD_CORE)

image: bootloader kernel programs
	@echo "Creating disk image..."
	@$(MAKE) -f image/image.mk BUILD_DIR=$(BUILD_IMAGE) \
		BOOTLOADER=$(BOOTLOADER_EFI) \
		KERNEL=$(KERNEL_BIN)

programs:
	@echo "Building user programs (shell)..."
	@$(MAKE) -C src/programs/shell -f shell.mk

# Run targets
run: image
	@echo "Running in QEMU..."
	@./scripts/run.sh $(IMAGE_TYPE) $(IMAGE_FILE)

debug: image
	@echo "Running in QEMU with GDB..."
	@./scripts/debug.sh $(IMAGE_TYPE) $(IMAGE_FILE)

bochs: image
	@echo "Running in Bochs..."
	@./scripts/bochs.sh $(IMAGE_TYPE) $(IMAGE_FILE)

# Toolchain setup
toolchain:
	@echo "Setting up cross-compiler toolchain..."
	@python3 ./scripts/setup_toolchain.py

# Clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Done."