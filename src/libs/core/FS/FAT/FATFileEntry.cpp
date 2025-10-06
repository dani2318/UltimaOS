#include "FATFileEntry.hpp"
#include <core/FS/FAT/FATFile.hpp>
#include <core/FS/FATFileSystem.hpp>

FATFileEntry::FATFileEntry() : fs(), DirectoryEntry() {}

void FATFileEntry::Initialize(FATFileSystem* fs, const FATDirectoryEntry& DirectoryEntry){
    this->fs = fs;
    this->DirectoryEntry = DirectoryEntry;
}

const char* FATFileEntry::Name() {
    return reinterpret_cast<const char*>(DirectoryEntry.Name);
}

const FileType FATFileEntry::Type(){
    if(DirectoryEntry.Attributes & FAT_ATTRIBUTE_DIRECTORY){
        return FileType::Directory;
    }

    return FileType::File;
}


File* FATFileEntry::Open(FileOpenMode mode){

    FATFile* file = fs->AllocateFile();

    if(file == nullptr)
        return nullptr;

    uint32_t size = DirectoryEntry.Size;
    uint32_t FirstCluster = DirectoryEntry.FirstClusterLow + ((uint32_t)DirectoryEntry.FirstClusterHigh << 16);
    if(!file->Open(fs, FirstCluster, Name(), size, DirectoryEntry.Attributes & FAT_ATTRIBUTE_DIRECTORY)){
        fs->ReleaseFile(file);
        return nullptr;
    }

    return file;
}

void FATFileEntry::Release() {
    fs->ReleaseFileEntry(this);
}
