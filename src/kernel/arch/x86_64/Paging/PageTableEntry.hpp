#pragma once

#include <stdint.h>

class PageTableEntry
{
public:
    void setFlag(uint64_t flag, bool enable)
    {
        if (enable) {
            value |= flag;
        } else {
            value &= ~flag;
}
    }

    bool getFlag(uint64_t flag) const
    {
        return (value & flag) != 0;
    }

    void setAddress(uint64_t address)
    {
        value &= 0xFFFULL;
        value |= address & 0x000FFFFFFFFFF000ULL;
    }

    uint64_t getAddress() const
    {
        return value & 0x000FFFFFFFFFF000ULL;
    }

    uint64_t value;
};