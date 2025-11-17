#pragma once
#include <core/memory/Allocator.hpp>
#include <stdint.h>

class Stage2Allocator : public Allocator{
    public:
        Stage2Allocator(void* base, uint32_t limit);
        virtual void* Allocate(size_t size) override;
        virtual void Free(void* addr) override;
    private:
        uint8_t* Base;
        uint32_t Allocated;
        uint32_t Limit;
};