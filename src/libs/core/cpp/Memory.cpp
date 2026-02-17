#include "Memory.hpp"

void * memset(void * ptr, int value, uint16_t num){
    uint8_t * u8Ptr = (uint8_t *)ptr;

    for(uint16_t i = 0; i < num; i++)
        u8Ptr[i] = (uint8_t)value;

    return ptr;
}