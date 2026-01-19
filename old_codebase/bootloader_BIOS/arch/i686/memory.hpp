#pragma once
#include <stdint.h>
#include <core/Defs.hpp>
#include "x86.h"

void * memcpy( void * dst, const void * src, size_t num);
void * memset(void * ptr, int value, size_t num);
int memcmp(const void * ptr1, const void * ptr2, size_t num);


EXPORT void* segmentoffset_to_linear(void* address);