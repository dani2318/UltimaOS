#pragma once
#include <stdint.h>
#include <core/FS/File.hpp>
#include <core/FS/FileEntry.hpp>
#include <core/FS/FAT/FATHeaders.hpp>

class FATFileSystem;
struct FATDirectoryEntry;

class FATFile : public File {
    public:
        FATFile();
        bool Open(FATFileSystem* fileSystem, uint32_t firstCluster,const char* name, uint32_t size, bool isDirectory);
        bool OpenFat1216RootDirectory(FATFileSystem* fileSystem, uint32_t rootDirectoryLba, uint32_t rootDirectorySize);
        
        bool isOpened() const {return this->Opened;};
        bool ReadFileEntry(FATDirectoryEntry* dirEntry);
        
        virtual void Release() override;
        virtual size_t Read(uint8_t* data, size_t size) override; 
        virtual size_t Write(const uint8_t* data, size_t size) override; 
        virtual bool Seek(SeekPos pos, int rel) override;
        virtual size_t Size() override { return size; };
        virtual size_t Position() override { return position; };
        virtual FileEntry* ReadFileEntry() override;
    private:
        bool UpdateCurrentCluster();
        uint8_t  Buffer[SectorSize];
        bool     Opened;
        bool     isRootDirectory;
        bool     isDirectory;
        uint32_t position;
        uint32_t size;
        uint32_t FirstCluster;
        uint32_t CurrentCluster;
        uint32_t CurrentClusterIndex;
        uint32_t CurrentSectorInCluster;
        FATFileSystem* fs;
};