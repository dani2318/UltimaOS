#pragma once
#include <stdint.h>
#include <core/Defs.hpp>
#include "x86.h"

void * memcpy( void * dst, const void * src, uint16_t num);
void * memset(void * ptr, int value, uint16_t num);
int memcmp(const void * ptr1, const void * ptr2, uint16_t num);


EXPORT void* segmentoffset_to_linear(void* address);