#include "Stage2Allocator.hpp"

Stage2Allocator::Stage2Allocator(void* base, uint32_t limit)
    :Base(reinterpret_cast<uint8_t*>(base)), Limit(limit){

}

void* Stage2Allocator::Allocate(size_t size){
    if(Allocated + size >= Limit)
        return nullptr;
    void* ret = Base + Allocated;
    Allocated += size;
    return ret;
}

void Stage2Allocator::Free(void* addr){
    //NO OP
}
