#pragma once
#include <stdint.h>

typedef struct{
    uint8_t  Attributes;
    uint8_t  ChsStart[3];
    uint8_t  PartitionType;
    uint8_t  ChsEnd[3];
    uint32_t LbaStart;
    uint32_t Size;
} __attribute__((packed)) MBREntry;

