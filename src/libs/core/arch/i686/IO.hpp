#pragma once
#include <stdint.h>
#include <core/Defs.hpp>

namespace arch{

    namespace i686{
        EXPORT void    ASMCALL         Outb(uint16_t port, uint8_t value);
        EXPORT uint8_t ASMCALL         Inb(uint16_t port);
        EXPORT void    ASMCALL         Outl(uint16_t port, uint32_t value);
        EXPORT uint8_t ASMCALL         Inl(uint16_t port);
        EXPORT void    ASMCALL         Cli();
        EXPORT void    ASMCALL         Sti();
        EXPORT void    ASMCALL         Panic();
        EXPORT void                    iowait();
    }
}