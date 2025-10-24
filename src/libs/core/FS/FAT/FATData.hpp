#pragma once
#include "FATHeaders.hpp"
#include "FATFile.hpp"
#include <stdint.h>
#include <core/memory/StaticObjectPool.hpp>
#include <core/FS/FAT/FATFileEntry.hpp>

constexpr int MaxFileHandles  =  10;
constexpr int MaxFileNameSize = 256;
constexpr int RootDirectoryHandle = -1;
constexpr int FatCacheSize    =  5;        // In sectors
constexpr uint32_t FatLfnLast = 0x40;

struct FATData
{
    union{
        FATBootSector BootSector;
        uint8_t       BootSectorBytes[SectorSize];
    } BS;

    FATFile RootDirectory;
    StaticObjectPool<FATFile, MaxFileHandles> OpenedFilePool;
    StaticObjectPool<FATFileEntry, MaxFileHandles> FileEntryPool;

    uint8_t  FatCache[FatCacheSize * SectorSize];
    uint32_t FatCachePosition;

    FATLFNBlock LFNBlocks[FatLfnLast];
    int LFNCount;

};