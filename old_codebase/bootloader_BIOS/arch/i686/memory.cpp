#include "memory.hpp"

EXPORT void* segmentoffset_to_linear(void* address){
    uint32_t offset = (uint32_t) (address) & 0xFFFF;
    uint32_t segment = (uint32_t) (address) >> 16;

    return (void*) (segment * 16) + offset;
}

