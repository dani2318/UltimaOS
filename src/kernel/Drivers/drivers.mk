# ============================================================================
# Drivers Makefile
# ============================================================================
# Builds driver object files (.o) for runtime loading
# Usage: make -f drivers.mk
# ============================================================================

# Root of drivers folder
DRIVERS_SRC_DIR := $(CURDIR)

# Output directory
BUILD_DIR := $(abspath ../../../../../build/x86_64_debug/drivers)

# Find all .cpp files in this directory (non-recursive)
DRIVERS_SRC := $(shell find $(DRIVERS_SRC_DIR) -maxdepth 1 -name '*.cpp')

# Map source files to object files
DRIVERS_OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(DRIVERS_SRC)))

# Compiler flags
CXXFLAGS := -ffreestanding -fno-stack-protector -fshort-wchar -I$(CURDIR)/../../../../..  # Include kernel headers
CXX := g++

.PHONY: all clean

all: $(DRIVERS_OBJ)

# Compile each driver
$(BUILD_DIR)/%.o: $(DRIVERS_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX -> OBJ] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning drivers..."
	@rm -rf $(BUILD_DIR)
