# ============================================================================
# Kernel Makefile
# ============================================================================
# This builds the kernel with C/C++/ASM sources and links with libcore
# Usage: make -f kernel.mk BUILD_DIR=build/x86_64_debug/kernel
# ============================================================================

include config.mk

# Build directory must be provided
ifndef BUILD_DIR
    $(error BUILD_DIR not defined. Usage: make -f kernel.mk BUILD_DIR=build/x86_64_debug/kernel)
endif

# BUILD_CORE should be provided, otherwise calculate it
ifndef BUILD_CORE
    BUILD_CORE := $(dir $(BUILD_DIR))libcore
    $(warning BUILD_CORE not provided, using: $(BUILD_CORE))
endif

# Source directory
SRC_DIR := src/kernel

# Recursively find all source files
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c' 2>/dev/null)
CPP_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' 2>/dev/null)
ASM_SOURCES := $(shell find $(SRC_DIR) -name '*.asm' 2>/dev/null)

# Generate object file paths
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

ALL_OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

# Output files
KERNEL_ELF := $(BUILD_DIR)/kernel.efi
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
KERNEL_MAP := $(BUILD_DIR)/kernel.map

# Linker script
LINKER_SCRIPT := $(SRC_DIR)/linker.ld

EDK2_ARCH := X64   # for 64-bit
EDK2_INC := -I$(EDK2_DIR)/MdePkg/Include \
            -I$(EDK2_DIR)/MdePkg/Include/Uefi \
            -I$(EDK2_DIR)/MdePkg/Include/Library \
            -I$(EDK2_DIR)/MdePkg/Include/Protocol \
            -I$(EDK2_DIR)/MdePkg/Include/X64

# Include paths
INCLUDES := -I$(SRC_DIR) -Isrc/libs/core -Isrc -I$(EDK2_DIR)/MdePkg/Include/Library

# Kernel-specific compilation flags
KERNEL_CFLAGS := -ffreestanding -fno-stack-protector $(ARCH_FLAGS)

# Linker flags
KERNEL_LDFLAGS := -T$(LINKER_SCRIPT) -Wl,-Map=$(KERNEL_MAP) -nostdlib -L$(BUILD_CORE)

# Libraries to link against
KERNEL_LIBS := -lcore -lgcc

# Add kernel-specific flags
CFLAGS += $(KERNEL_CFLAGS) $(INCLUDES) $(EDK2_INC) -fshort-wchar
CXXFLAGS += $(KERNEL_CFLAGS) $(INCLUDES) $(EDK2_INC) -fshort-wchar
ASFLAGS += -I$(SRC_DIR)

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean info check-deps drivers

DRIVERS_MK := $(SRC_DIR)/arch/x86_64/Drivers/drivers.mk

all: check-deps $(KERNEL_BIN)

drivers:
	@$(MAKE) -f $(DRIVERS_MK)

# Check for dependencies
check-deps:
	@if [ ! -f "$(LINKER_SCRIPT)" ]; then \
		echo "Error: Linker script not found: $(LINKER_SCRIPT)"; \
		exit 1; \
	fi
	@if [ ! -f "$(BUILD_CORE)/libcore.a" ]; then \
		echo "Error: libcore.a not found. Build it first with: make libcore"; \
		exit 1; \
	fi

# Link kernel ELF binary
$(KERNEL_ELF): $(ALL_OBJECTS)
	@mkdir -p $(dir $@)
	@echo "[LD]      $@"
	@$(LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o $@ $^ $(KERNEL_LIBS)
	@echo "✓ Kernel ELF linked: $@"
	@echo "  Map file: $(KERNEL_MAP)"

# Convert ELF to flat binary
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] $@"
	@$(OBJCOPY) -O binary $< $@
	@echo "✓ Kernel binary created: $@"
	@ls -lh $@ | awk '{print "  Size: " $$5}'

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX]     $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Assemble ASM sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "[AS]      $<"
	@$(AS) $(ASFLAGS) $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning kernel..."
	@rm -rf $(BUILD_DIR)

# Display build info
info:
	@echo "=== Kernel Build Info ==="
	@echo "Source Dir:     $(SRC_DIR)"
	@echo "Build Dir:      $(BUILD_DIR)"
	@echo "Architecture:   $(ARCH)"
	@echo "Config:         $(CONFIG)"
	@echo "Output ELF:     $(KERNEL_ELF)"
	@echo "Output Binary:  $(KERNEL_BIN)"
	@echo "Linker Script:  $(LINKER_SCRIPT)"
	@echo "C Sources:      $(words $(C_SOURCES))"
	@echo "C++ Sources:    $(words $(CPP_SOURCES))"
	@echo "ASM Sources:    $(words $(ASM_SOURCES))"
	@echo "Total Objects:  $(words $(ALL_OBJECTS))"
	@echo "Compiler:       $(CC)"
	@echo "Linker:         $(LD)"

# Debug helper
print-%:
	@echo '$*=$($*)'