# ============================================================================
# Core Library Makefile
# ============================================================================
# This builds the core library (libcore.a) with C/C++/ASM sources
# Usage: make -f core.mk BUILD_DIR=build/x86_64_debug/libcore
# ============================================================================

include config.mk

# Build directory must be provided
ifndef BUILD_DIR
    $(error BUILD_DIR not defined. Usage: make -f core.mk BUILD_DIR=build/x86_64_debug/libcore)
endif

# Source directory
SRC_DIR := src/libs/core

# Recursively find all source files
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c' 2>/dev/null)
CPP_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' 2>/dev/null)
ASM_SOURCES := $(shell find $(SRC_DIR) -name '*.asm' 2>/dev/null)

# Generate object file paths maintaining directory structure
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

ALL_OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

# Output library
OUTPUT := $(BUILD_DIR)/libcore.a

# Include paths
INCLUDES := -I$(SRC_DIR)

# Add includes to flags
CFLAGS += $(INCLUDES)
CXXFLAGS += $(INCLUDES)
ASFLAGS += -I$(SRC_DIR)

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean info

all: $(OUTPUT)

# Archive object files into static library
$(OUTPUT): $(ALL_OBJECTS)
	@mkdir -p $(dir $@)
	@echo "[AR]      $@"
	@$(AR) rcs $@ $^
	@echo "✓ Core library built: $@"

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
	@echo "Cleaning libcore..."
	@rm -rf $(BUILD_DIR)

# Display build info
info:
	@echo "=== Core Library Build Info ==="
	@echo "Source Dir:    $(SRC_DIR)"
	@echo "Build Dir:     $(BUILD_DIR)"
	@echo "Output:        $(OUTPUT)"
	@echo "C Sources:     $(words $(C_SOURCES))"
	@echo "C++ Sources:   $(words $(CPP_SOURCES))"
	@echo "ASM Sources:   $(words $(ASM_SOURCES))"
	@echo "Total Objects: $(words $(ALL_OBJECTS))"
	@echo "Compiler:      $(CC)"
	@echo "Archiver:      $(AR)"

# Debug helper - print any variable
print-%:
	@echo '$*=$($*)'