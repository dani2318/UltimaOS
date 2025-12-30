#include <arch/x86_64/Paging.hpp>

void PageTableEntry::SetFlag(uint64_t flag, bool enable) {
    if(enable) value |= flag;
    else value &= ~flag;
}

bool PageTableEntry::GetFlag(uint64_t flag) {
    return (value & flag) > 0;
}

void PageTableEntry::SetAddress(uint64_t address) {
    address &= 0x000000ffffffffff;
    value &= 0xfff0000000000fff;
    value |= address;
}

uint64_t PageTableEntry::GetAddress() {
    return value & 0x000000ffffffffff;
} 


