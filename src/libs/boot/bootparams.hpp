#pragma once

#include <stdint.h>

struct MemoryRegion{
    uint64_t Begin; 
    uint64_t Length;
    uint32_t Type;
    uint32_t ACPI;
} ;

struct  MemoryInfo{
    int RegionCount;
    MemoryRegion* Regions;
} ;

struct BootParameters{
    MemoryInfo Memory;
    uint8_t BootDevice;
} ;