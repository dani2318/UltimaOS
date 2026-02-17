#include "NewDelete.hpp"

Allocator* g_cppAlloc;

void SetCppAlloc(Allocator* cppAlloc){
    g_cppAlloc = cppAlloc;
};


void* operator new(size_t size) throw(){
    if(!g_cppAlloc)
        return nullptr;
    return g_cppAlloc->Allocate(size);
}

void* operator new[](size_t size) throw(){
    if(!g_cppAlloc)
        return nullptr;
    return g_cppAlloc->Allocate(size);
}

void operator delete(void* ptr){
    if(g_cppAlloc)
        g_cppAlloc->Free(ptr);
}

void operator delete[](void* ptr){
    if(g_cppAlloc)
        g_cppAlloc->Free(ptr);
}