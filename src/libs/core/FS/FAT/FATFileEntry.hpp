#pragma once

#include <core/FS/FileEntry.hpp>
#include <core/FS/File.hpp>
#include <core/FS/FAT/FATHeaders.hpp>
class FATFileSystem;

class FATFileEntry : public FileEntry{
    public:
        FATFileEntry();
        void Initialize(FATFileSystem* fs, const FATDirectoryEntry& DirectoryEntry);
        virtual const char* Name() override;
        virtual const FileType Type() override;
        virtual File* Open(FileOpenMode mode) override;
        virtual void Release() override;
    private:
        FATDirectoryEntry DirectoryEntry;
        FATFileSystem* fs;
};