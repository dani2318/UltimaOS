#pragma once
#include <stdbool.h>
#include <stdint.h>

// typedef struct{
//     const char* Name;
//     bool (*Probe)();
//     void (*Initialize)(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEOI);
//     void (*Disable)();
//     void (*SendEOI)(int irq);
//     void (*Mask)(int irq);
//     void (*Unmask)(int irq);
// } PICDriver;

class PICDriver{
    public:
        constexpr PICDriver(
        bool (*probe)(const PICDriver* self),
        void (*init)(uint8_t,uint8_t,bool),
        void (*disable)(),
        void (*eoi)(int),
        void (*mask)(int),
        void (*unmask)(int),
        const char* name
    )
        : Probe(probe),
          Initialize(init),
          Disable(disable),
          SendEOI(eoi),
          Mask(mask),
          Unmask(unmask),
          name(name)
    {}

        bool (*Probe)(const PICDriver* self);
        void (*Initialize)(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEOI);
        void (*Disable)();
        void (*SendEOI)(int irq);
        void (*Mask)(int irq);
        void (*Unmask)(int irq);
        const char* name;
};