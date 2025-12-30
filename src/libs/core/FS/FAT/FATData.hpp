#pragma once
#include "FATHeaders.hpp"
#include "FATFile.hpp"
#include <core/memory/StaticObjectPool.hpp>
#include <core/FS/FAT/FATHeaders.hpp>
#include <core/FS/FAT/FATFileEntry.hpp>

constexpr int MaxFileNameSize = 256;
constexpr int MaxFileHandles = 10;
constexpr int RootDirectoryHandle = -1;
constexpr int FatCacheSize = 5;

constexpr uint32_t FATLFNLast = 0x40;
class FATFile;

typedef struct
{
    union
    {
        FATBootSector BootSector;
        uint8_t BootSectorBytes[SectorSize];
    } BS;

    FATFile RootDirectory;
    StaticObjectPool<FATFile, MaxFileHandles> OpenedFilePool;
    StaticObjectPool<FATFileEntry, MaxFileHandles> FileEntryPool;

    uint8_t FatCache[FatCacheSize * SectorSize];
    uint32_t FatCachePosition;

    FATLFNBlock LFNBlocks[FATLFNLast];
    int LFNCount;

} FATData;