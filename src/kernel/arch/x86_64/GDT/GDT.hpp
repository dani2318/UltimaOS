#pragma once 
#include <stdint.h>
#include <globals.hpp>

// typedef struct {
//     uint16_t limit_low;
//     uint16_t base_low;
//     uint8_t  base_middle;
//     uint8_t  access;
//     uint8_t  attributes;
//     uint8_t  base_high;
// } __attribute__((packed)) GDTEntry;

// typedef struct {
//     uint16_t limit;
//     uint64_t base;
// } __attribute__((packed)) GDTDescriptor;

// constexpr uint8_t GDT_ACCESS_CODE = 0b10011010;
// constexpr uint8_t GDT_ACCESS_DATA = 0b10010010;
// constexpr uint8_t GDT_FLAGS_CODE = 0b00100000;
// constexpr uint8_t GDT_FLAGS_DATA = 0b00000000;



typedef struct{
    uint16_t    LimitLow;
    uint16_t    BaseLow;
    uint8_t     BaseMiddle;
    uint8_t     Access;
    uint8_t     FlagsLimitHi;
    uint8_t     BaseHigh;
} __attribute__((packed)) GDTEntry;

typedef struct{
    uint16_t Limit;
    GDTEntry* ptr;
} __attribute__((packed)) GDTDescriptor;


typedef enum{
    GDT_ACCESS_CODE_READABLE                    = 0x02,
    GDT_ACCESS_CODE_WRITABLE                    = 0x02,

    GDT_ACCESS_CODE_CONFORMING                  = 0x04,
    GDT_ACCESS_CODE_DIRECTION_NORMAL            = 0x00,
    GDT_ACCESS_CODE_DIRECTION_DOWN              = 0x04,

    GDT_ACCESS_DATA_SEGMENT                     = 0x10,
    GDT_ACCESS_CODE_SEGMENT                     = 0x18,

    GDT_ACCESS_DESCRIPTOR_TSS                   = 0x00,

    GDT_ACCESS_RING0                            = 0x00,
    GDT_ACCESS_RING1                            = 0x20,
    GDT_ACCESS_RING2                            = 0x40,
    GDT_ACCESS_RING3                            = 0x60,

    GDT_ACCESS_PRESENT                          = 0x80,

} GDT_ACCESS;


typedef enum{

    GDT_FLAG_64BIT                              = 0x20,
    GDT_FLAG_32BIT                              = 0x40,
    GDT_FLAG_16BIT                              = 0x00,

    GDT_FLAG_GRANULARITY_1B                     = 0x00,
    GDT_FLAG_GRANULARITY_4K                     = 0x80,

} GDT_FLAGS;


#define i686_GDT_CODE_SEGMENT 0x08
#define i686_GDT_DATA_SEGMENT 0x10
class GDT {
    public:
        void Init();
};
