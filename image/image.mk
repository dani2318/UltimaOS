# ============================================================================
# Disk Image Makefile
# ============================================================================
# This creates a bootable UEFI disk image with GPT partition table
# Usage: make -f image.mk BUILD_DIR=build/x86_64_debug/image \
#        BOOTLOADER=path/to/BOOTX64.EFI KERNEL=path/to/kernel.bin
# ============================================================================

include config.mk

# Build directory must be provided
ifndef BUILD_DIR
    $(error BUILD_DIR not defined. Usage: make -f image.mk BUILD_DIR=build/x86_64_debug/image)
endif

# Input files must be provided
ifndef BOOTLOADER
    $(error BOOTLOADER not defined. Provide path to BOOTX64.EFI)
endif

ifndef KERNEL
    $(error KERNEL not defined. Provide path to kernel.bin)
endif

# Output image
IMAGE_FILE := $(BUILD_DIR)/os_image.img

# Image configuration
SECTOR_SIZE := 512
PARTITION_OFFSET := 2048
GPT_BACKUP_SIZE := 34

# Calculate size in sectors
IMAGE_SIZE_BYTES := $(shell echo $(IMAGE_SIZE) | sed 's/[^0-9]//g')000000
IMAGE_SIZE_SECTORS := $(shell echo $$(($(IMAGE_SIZE_BYTES) / $(SECTOR_SIZE))))
END_SECTOR := $(shell echo $$(($(IMAGE_SIZE_SECTORS) - $(GPT_BACKUP_SIZE))))

# Partition offset in bytes for mtools
PARTITION_OFFSET_BYTES := $(shell echo $$(($(PARTITION_OFFSET) * $(SECTOR_SIZE))))

# Temporary files
MTOOLS_CONF := $(BUILD_DIR)/.mtoolsrc
LOOPBACK_FILE := $(BUILD_DIR)/.loopback

# Font file (if exists)
FONT_FILE := src/assets/zap-light16.psf

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean info check-deps mount umount

all: check-deps $(IMAGE_FILE)

# Check for dependencies
check-deps:
	@echo "Checking dependencies..."
	@if [ ! -f "$(BOOTLOADER)" ]; then \
		echo "✗ Error: Bootloader not found: $(BOOTLOADER)"; \
		exit 1; \
	fi
	@if [ ! -f "$(KERNEL)" ]; then \
		echo "✗ Error: Kernel not found: $(KERNEL)"; \
		exit 1; \
	fi
	@command -v parted >/dev/null 2>&1 || { \
		echo "✗ Error: parted not found. Install: sudo apt install parted"; \
		exit 1; \
	}
	@command -v mkfs.fat >/dev/null 2>&1 || { \
		echo "✗ Error: mkfs.fat not found. Install: sudo apt install dosfstools"; \
		exit 1; \
	}
	@command -v mtools >/dev/null 2>&1 || { \
		echo "✗ Error: mtools not found. Install: sudo apt install mtools"; \
		exit 1; \
	}
	@echo "✓ All dependencies found"

# Create the bootable UEFI disk image
$(IMAGE_FILE): $(BOOTLOADER) $(KERNEL)
	@mkdir -p $(BUILD_DIR)
	@echo "=========================================="
	@echo "Creating UEFI Disk Image"
	@echo "=========================================="
	@echo "Image size:    $(IMAGE_SIZE) ($(IMAGE_SIZE_SECTORS) sectors)"
	@echo "Bootloader:    $(BOOTLOADER)"
	@echo "Kernel:        $(KERNEL)"
	@echo ""
	
	@# 1. Create empty disk image
	@echo "[1/5] Creating empty disk image..."
	@dd if=/dev/zero of=$@ bs=$(SECTOR_SIZE) count=$(IMAGE_SIZE_SECTORS) status=none
	@echo "      ✓ Image file created: $@"
	
	@# 2. Create GPT partition table
	@echo "[2/5] Creating GPT partition table..."
	@parted -s $@ mklabel gpt
	@parted -s $@ mkpart ESP fat32 $(PARTITION_OFFSET)s $(END_SECTOR)s
	@parted -s $@ set 1 esp on
	@sync
	@echo "      ✓ GPT table created with ESP partition"
	
	@# 3. Format partition as FAT32
	@echo "[3/5] Formatting ESP as FAT32..."
	@mkfs.fat -F 32 -n "ESP" --offset $(PARTITION_OFFSET) -S $(SECTOR_SIZE) $@ >/dev/null 2>&1
	@sync
	@echo "      ✓ FAT32 filesystem created"
	
	@# 4. Setup mtools config
	@echo "[4/5] Setting up mtools configuration..."
	@echo "drive x: file=\"$@\" offset=$(PARTITION_OFFSET_BYTES)" > $(MTOOLS_CONF)
	@echo "      ✓ mtools configured"

	@# 5. Copy files using mtools
	@echo "[5/5] Copying bootloader, kernel, and user programs..."
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/EFI 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/EFI/BOOT 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/NEOOS 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/PROGRAMS 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(BOOTLOADER) x:/EFI/BOOT/BOOTX64.EFI
	@MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(KERNEL) x:/NEOOS/KERNEL.BIN
	@if [ -f "build/x86_64_debug/programs/shell.elf" ]; then \
		MTOOLSRC=$(MTOOLS_CONF) mcopy -o build/x86_64_debug/programs/shell.elf x:/PROGRAMS/SHELL.ELF; \
	fi

	@if [ -f "$(FONT_FILE)" ]; then \
		echo "      Copying font file..."; \
		MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(FONT_FILE) x:/NEOOS/FONT.PSF; \
	fi

	@rm -f $(MTOOLS_CONF)
	@echo "      ✓ Files copied to image"
	@echo ""
	@echo "=========================================="
	@echo "✓ UEFI Disk Image Created Successfully"
	@echo "=========================================="
	@ls -lh $@ | awk '{print "Size: " $$5}'
	@echo "Location: $@"

# Alternative: Simple flat FAT32 image (no partitions, for testing)
simple: check-deps
	@mkdir -p $(BUILD_DIR)
	@echo "Creating simple FAT32 image (no GPT)..."
	@dd if=/dev/zero of=$(IMAGE_FILE) bs=1M count=64 status=none
	@mkfs.fat -F 32 -n "OSBOOT" $(IMAGE_FILE) >/dev/null 2>&1
	@echo "drive x: file=\"$(IMAGE_FILE)\"" > $(MTOOLS_CONF)
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/EFI 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/EFI/BOOT 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mmd -D O x:/NEOOS 2>/dev/null || true
	@MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(BOOTLOADER) x:/EFI/BOOT/BOOTX64.EFI
	@MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(KERNEL) x:/NEOOS/KERNEL.BIN
	@if [ -f "$(FONT_FILE)" ]; then \
		MTOOLSRC=$(MTOOLS_CONF) mcopy -o $(FONT_FILE) x:/NEOOS/FONT.PSF; \
	fi
	@rm -f $(MTOOLS_CONF)
	@echo "✓ Simple image created: $(IMAGE_FILE)"

# Mount the image (requires loop device support)
mount:
	@mkdir -p $(BUILD_DIR)/mount
	@if [ ! -f "$(IMAGE_FILE)" ]; then \
		echo "✗ Error: Image not found: $(IMAGE_FILE)"; \
		exit 1; \
	fi
	@echo "Mounting $(IMAGE_FILE)..."
	@LODEV=$$(sudo losetup --show -fP $(IMAGE_FILE)); \
	echo $$LODEV > $(LOOPBACK_FILE); \
	sudo mount $${LODEV}p1 $(BUILD_DIR)/mount
	@echo "✓ Mounted at $(BUILD_DIR)/mount"
	@echo "  Loopback device: $$(cat $(LOOPBACK_FILE))"

# Unmount the image
umount:
	@if [ ! -f "$(LOOPBACK_FILE)" ]; then \
		echo "No loopback device file found"; \
		exit 0; \
	fi
	@LODEV=$$(cat $(LOOPBACK_FILE)); \
	if [ -n "$$LODEV" ]; then \
		echo "Unmounting $$LODEV..."; \
		sudo umount $(BUILD_DIR)/mount 2>/dev/null || true; \
		sudo losetup -d $$LODEV 2>/dev/null || true; \
		rm -f $(LOOPBACK_FILE); \
		echo "✓ Unmounted"; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning image build..."
	@$(MAKE) -f image.mk umount 2>/dev/null || true
	@rm -rf $(BUILD_DIR)
	@echo "✓ Cleaned"

# Display build info
info:
	@echo "=== Disk Image Build Info ==="
	@echo "Build Dir:        $(BUILD_DIR)"
	@echo "Output:           $(IMAGE_FILE)"
	@echo "Bootloader:       $(BOOTLOADER)"
	@echo "Kernel:           $(KERNEL)"
	@echo "Font:             $(FONT_FILE)"
	@echo ""
	@echo "Image Configuration:"
	@echo "  Size:           $(IMAGE_SIZE)"
	@echo "  Filesystem:     $(IMAGE_FS)"
	@echo "  Type:           $(IMAGE_TYPE)"
	@echo "  Sectors:        $(IMAGE_SIZE_SECTORS)"
	@echo "  Partition:      $(PARTITION_OFFSET) - $(END_SECTOR)"
	@echo ""
	@echo "Tools:"
	@command -v parted >/dev/null 2>&1 && echo "  ✓ parted" || echo "  ✗ parted"
	@command -v mkfs.fat >/dev/null 2>&1 && echo "  ✓ mkfs.fat" || echo "  ✗ mkfs.fat"
	@command -v mtools >/dev/null 2>&1 && echo "  ✓ mtools" || echo "  ✗ mtools"

# List contents of the image
list: check-deps
	@if [ ! -f "$(IMAGE_FILE)" ]; then \
		echo "✗ Error: Image not found: $(IMAGE_FILE)"; \
		exit 1; \
	fi
	@echo "Contents of $(IMAGE_FILE):"
	@echo "drive x: file=\"$(IMAGE_FILE)\" offset=$(PARTITION_OFFSET_BYTES)" > $(MTOOLS_CONF)
	@MTOOLSRC=$(MTOOLS_CONF) mdir -/ x:
	@rm -f $(MTOOLS_CONF)

# Debug helper
print-%:
	@echo '$*=$($*)'