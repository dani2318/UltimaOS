#pragma once
#include <stdint.h>
#include <stddef.h>
#include <core/Defs.hpp>
#include <core/Debug.hpp>
#include <boot/bootparams.hpp>

EXPORT void* ASMCALL memcpy(void* dest, const void* src, size_t count);
EXPORT void * memset(void * ptr, int value, uint16_t num);
namespace Memory
{
    constexpr auto Copy = memcpy;
    constexpr auto Set = memset;
} // namespace Memory
