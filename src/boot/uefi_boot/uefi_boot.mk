# ============================================================================
# UEFI Bootloader Makefile
# ============================================================================
# This builds the UEFI bootloader using gnu-efi
# Usage: make -f uefi_boot.mk BUILD_DIR=build/x86_64_debug/uefi_boot
# ============================================================================

include config.mk

# Build directory must be provided
ifndef BUILD_DIR
    $(error BUILD_DIR not defined. Usage: make -f uefi_boot.mk BUILD_DIR=build/x86_64_debug/uefi_boot)
endif

# Source directory
SRC_DIR := src/boot/uefi_boot

# Recursively find all C source files
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c' 2>/dev/null)

# Generate object file paths
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Output files
BOOTLOADER_SO := $(BUILD_DIR)/BOOTX64.so
BOOTLOADER_EFI := $(BUILD_DIR)/BOOTX64.EFI

# Linker script - ALWAYS use system gnu-efi linker script for compatibility
# Custom linker scripts often cause issues with gnu-efi's crt0
UEFI_LDS := /usr/lib/elf_x86_64_efi.lds

# Check if user has custom linker script
CUSTOM_LDS := $(SRC_DIR)/linker.ld
ifneq ($(wildcard $(CUSTOM_LDS)),)
    $(warning Custom linker script found at $(CUSTOM_LDS) but using system script for compatibility)
    $(warning To use custom script, you must also build custom crt0)
endif

# UEFI-specific compilation flags
UEFI_CFLAGS := \
    -ffreestanding \
    -fno-stack-protector \
    -fshort-wchar \
    -mno-red-zone \
    -fPIC \
    -fno-stack-check

# UEFI include paths
UEFI_INCLUDES := \
    -I$(UEFI_INCLUDE) \
    -I$(UEFI_INCLUDE)/x86_64 \
    -I$(UEFI_INCLUDE)/protocol \
    -I$(SRC_DIR) \
    -Isrc/libs/core \
    -Isrc

# UEFI linker flags (for direct ld invocation)
UEFI_LDFLAGS := \
    -nostdlib \
    -znocombreloc \
    -T $(UEFI_LDS) \
    -shared \
    -Bsymbolic \
    -L $(UEFI_LIB) \
    $(UEFI_CRT0)

# UEFI libraries
UEFI_LIBS := -lefi -lgnuefi

# objcopy flags for EFI conversion
OBJCOPY_FLAGS := \
    -j .text \
    -j .sdata \
    -j .data \
    -j .dynamic \
    -j .dynsym \
    -j .rel \
    -j .rela \
    -j .rel.* \
    -j .rela.* \
    -j .reloc \
    --target=efi-app-x86_64

# Override compiler tools to use SYSTEM gcc/objcopy for UEFI (not cross-compiler)
UEFI_CC := gcc
UEFI_LD := ld
UEFI_OBJCOPY := objcopy

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean info check-deps

all: check-deps $(BOOTLOADER_EFI)

# Check for UEFI development dependencies
check-deps:
	@echo "Checking UEFI dependencies..."
	@if [ ! -f "$(UEFI_CRT0)" ]; then \
		echo "✗ Error: $(UEFI_CRT0) not found"; \
		echo "  Install gnu-efi: sudo apt install gnu-efi"; \
		exit 1; \
	fi
	@if [ ! -f "$(UEFI_LDS)" ]; then \
		echo "✗ Error: $(UEFI_LDS) not found"; \
		echo "  Install gnu-efi: sudo apt install gnu-efi"; \
		exit 1; \
	fi
	@if [ -z "$(C_SOURCES)" ]; then \
		echo "✗ Error: No .c files found in $(SRC_DIR)"; \
		exit 1; \
	fi
	@echo "✓ UEFI dependencies found"
	@echo "  Using system linker script: $(UEFI_LDS)"

# Link ELF shared object using ld directly
$(BOOTLOADER_SO): $(C_OBJECTS)
	@mkdir -p $(dir $@)
	@echo "[LD]      $@"
	@$(UEFI_LD) $(UEFI_LDFLAGS) $^ -o $@ $(UEFI_LIBS)
	@echo "✓ UEFI shared object linked: $@"

# Convert ELF shared object to EFI application
$(BOOTLOADER_EFI): $(BOOTLOADER_SO)
	@echo "[OBJCOPY] $@"
	@$(UEFI_OBJCOPY) $(OBJCOPY_FLAGS) $< $@
	@echo "✓ UEFI bootloader created: $@"
	@ls -lh $@ | awk '{print "  Size: " $5}'

# Compile C sources with UEFI-specific flags
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]      $<"
	@$(UEFI_CC) $(CFLAGS) $(UEFI_CFLAGS) $(UEFI_INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning UEFI bootloader..."
	@rm -rf $(BUILD_DIR)

# Display build info
info:
	@echo "=== UEFI Bootloader Build Info ==="
	@echo "Source Dir:      $(SRC_DIR)"
	@echo "Build Dir:       $(BUILD_DIR)"
	@echo "Output:          $(BOOTLOADER_EFI)"
	@echo "C Sources:       $(words $(C_SOURCES))"
	@echo "Total Objects:   $(words $(C_OBJECTS))"
	@echo "Compiler:        $(UEFI_CC) (system gcc, not cross-compiler)"
	@echo "Objcopy:         $(UEFI_OBJCOPY)"
	@echo "UEFI Include:    $(UEFI_INCLUDE)"
	@echo "UEFI CRT0:       $(UEFI_CRT0)"
	@echo "Linker Script:   $(UEFI_LDS) (system)"
	@echo ""
	@echo "Dependencies:"
	@if [ -f "$(UEFI_CRT0)" ]; then \
		echo "  ✓ crt0-efi-x86_64.o"; \
	else \
		echo "  ✗ crt0-efi-x86_64.o (missing)"; \
	fi
	@if [ -f "$(UEFI_LDS)" ]; then \
		echo "  ✓ Linker script"; \
	else \
		echo "  ✗ Linker script (missing)"; \
	fi
	@if [ -f "$(CUSTOM_LDS)" ]; then \
		echo "  ℹ Custom linker script exists but not used (requires custom crt0)"; \
	fi

# Debug helper
print-%:
	@echo '$*=$($*)'