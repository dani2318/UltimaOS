#pragma once
#include <stddef.h>
#include <stddef.h>
#include <stdbool.h>

template<typename T, size_t PoolSize>
class StaticObjectPool{

    public:
        StaticObjectPool();
        T* Allocate();
        void Free(T* object);
    private:
        T Objects[PoolSize] = {};
        bool ObjectInUse[PoolSize] = {};
        size_t size;

};

template<typename T, size_t PoolSize>
StaticObjectPool<T, PoolSize>::StaticObjectPool()
    : size(0)
{

    for(size_t i = 0; i < PoolSize; i++)
        ObjectInUse[i] = false;

}

template<typename T, size_t PoolSize>
T* StaticObjectPool<T, PoolSize>::Allocate(){
    if( size >= PoolSize)
        return nullptr;

    for(size_t i = 0; i < PoolSize; i++){
        size_t idx = (i + size) % PoolSize;
        if(!ObjectInUse[idx]){
            ObjectInUse[idx] = true;
            size++;
            return &Objects[idx];
        }
    }
    return nullptr;
}

template<typename T, size_t PoolSize>
void StaticObjectPool<T, PoolSize>::Free(T* object){
    if(object < Objects || object >= Objects + PoolSize)
        return;
    size_t idx = object - Objects;
    ObjectInUse[idx] = false;
    size--;
}