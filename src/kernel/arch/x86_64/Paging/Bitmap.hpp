#pragma once
#include <stdint.h>
#include <stddef.h>

class Bitmap
{
public:
    bool get(uint64_t index) const
    {
        if (index >= size * 8)
            return true;
        return (buffer[index / 8] & (1 << (index % 8))) != 0;
    }

    void set(uint64_t index, bool value) const
    {
        if (index >= size * 8)
            return;
        if (value) {
            buffer[index / 8] |= (1 << (index % 8));
        } else {
            buffer[index / 8] &= ~(1 << (index % 8));
}
    }

    size_t size;
    uint8_t *buffer;
};