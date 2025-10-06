#include "FATFile.hpp"
#include <core/FS/FATFileSystem.hpp>
#include <core/Debug.hpp>
#include <core/cpp/Algorithm.hpp>
#include <core/memory/Allocator.hpp>
#include <core/FS/FAT/FATHeaders.hpp>
#include <core/cpp/Memory.hpp>

constexpr const char* module_name = "FATFile";

FATFile::FATFile()
    :   fs(nullptr),
        Opened(false),
        isRootDirectory(false),
        position(),
        size(),
        FirstCluster(),
        CurrentCluster(),
        CurrentClusterIndex(0),
        CurrentSectorInCluster()
{
}

/// @brief This is used to read files for FAT32 FileSystems
/// @param fileSystem       This should be a pointer to the filesystem
/// @param firstCluster     The fistcluster to read
/// @param size             The size to read
/// @return A boolean that tell if the operation is compleated correctly
bool FATFile::Open(FATFileSystem* fileSystem, uint32_t firstCluster,const char* name, uint32_t size, bool isDirectory){
    
    this->isRootDirectory = false;
    this->position = 0;
    this->size = size;
    this->FirstCluster = firstCluster;
    this->isDirectory = isDirectory;
    this->CurrentCluster = this->FirstCluster;
    this->CurrentClusterIndex = 0;
    this->CurrentSectorInCluster = 0;

    if (!fileSystem->ReadSectorFromCluster(this->FirstCluster, CurrentSectorInCluster, this->Buffer)){
        Debug::Error(module_name, "Failed to open file %s!!", name);
        return false;
    }
    this->Opened = true;

    return true;
}
/// @brief This is used to read the root directory in the FAT12 or FAT16 Filesystem
/// @param fileSystem           This should be a pointer to the filesystem
/// @param rootDirectoryLba     Start LBA of the root directory
/// @param rootDirectorySize    Size of the root directory
/// @return A boolean that tell if the operation is compleated correctly
bool FATFile::OpenFat1216RootDirectory(FATFileSystem* fileSystem, uint32_t rootDirectoryLba, uint32_t rootDirectorySize){
    this->isRootDirectory = true;
    this->position = 0;
    this->size = rootDirectorySize;
    this->FirstCluster = rootDirectoryLba;
    this->CurrentCluster = this->FirstCluster;
    this->CurrentSectorInCluster = 0;

    if (!fileSystem->ReadSector(this->FirstCluster, this->Buffer)){
        Debug::Error(module_name,"Read root directory failed!!");
        return false;
    }
    return true;
}

/// @brief Reads a File entry
/// @param dirEntry       FATDirectoryEntry
/// @return A boolean that tell if the operation is compleated correctly
bool FATFile::ReadFileEntry(FATDirectoryEntry* dirEntry){
    return Read(reinterpret_cast<uint8_t*>(dirEntry), sizeof(FATDirectoryEntry)) == sizeof(FATDirectoryEntry);
}

uint32_t FATFile::Read(uint8_t* data, size_t byteCount){
    uint8_t* original_data = data;
    // don't read past the end of the file
    if (!isDirectory || (isDirectory && size != 0))
        byteCount = min(byteCount, size - position);

    while(byteCount > 0){
        uint32_t leftInBuffer = SectorSize - (position % SectorSize);
        uint32_t take = min(byteCount,leftInBuffer);

        Memory::Copy(data, this->Buffer + position % SectorSize, take);
        data += take;
        position += take;
        byteCount -= take;
        if(leftInBuffer == take){
            if(isRootDirectory){
                ++CurrentCluster;
                if(!fs->ReadSector(CurrentCluster, this->Buffer)){
                    Debug::Error(module_name,"Read error!!");
                    break;
                }
            } else {
                if(++CurrentSectorInCluster >= fs->GetFatData().BS.BootSector.SectorsPerCluster){
                    CurrentSectorInCluster = 0;
                    CurrentCluster = fs->GetNextCluster(CurrentCluster);
                    ++CurrentClusterIndex;
                }

                if(CurrentCluster >= 0xFFFFFFF8){
                    size = position;
                    break;
                }

                if(!fs->ReadSectorFromCluster(CurrentCluster + CurrentSectorInCluster, 1, this->Buffer)){
                    Debug::Error(module_name,"Read error!!");
                    break;
                }
            }
        }
    }

    return data - original_data;
}
bool FATFile::Seek(SeekPos pos, int rel) {

    switch (pos)
    {
        case SeekPos::Set:
            position = static_cast<uint32_t>(max(0,rel));
            break;
        case SeekPos::Current:
            if(rel < 0 && position < -rel) position = 0;
            position = min(Size(), static_cast<uint32_t>(position + rel));
            break;
        case SeekPos::End:
            if(rel < 0 && Size() < -rel) position = 0;
            position = min(Size(), static_cast<uint32_t>(position + rel));
            break;
    }
    return UpdateCurrentCluster();
}

bool FATFile::UpdateCurrentCluster(){
    uint32_t ClusterSize = fs->GetFatData().BS.BootSector.SectorsPerCluster * SectorSize;
    uint32_t desiredCluster = position / ClusterSize;
    uint32_t desiredSector = (position % ClusterSize) / SectorSize;

    if (desiredCluster == CurrentClusterIndex && desiredSector == CurrentSectorInCluster){
        return true;
    }

    if(desiredCluster < CurrentClusterIndex){
        CurrentClusterIndex = 0;
        CurrentCluster = FirstCluster;
    }

    while (desiredCluster > CurrentClusterIndex){
        CurrentCluster = fs->GetNextCluster(CurrentCluster);
        ++CurrentClusterIndex;
    }

    CurrentSectorInCluster = desiredSector;
    return fs->ReadSectorFromCluster(CurrentCluster, CurrentSectorInCluster, this->Buffer);
}

FileEntry* FATFile::ReadFileEntry(){
    FATDirectoryEntry entry;
    if(ReadFileEntry(&entry)){
        FATFileEntry* fileEntry = fs->AllocateFileEntry();
        fileEntry->Initialize(fs, entry);
        return fileEntry;
    }
    return nullptr;
}

void FATFile::Release() {
    fs->ReleaseFile(this);
}

/// @brief Unsupported!!! THIS FUNCTION IS NOT YET IMPLEMENTED!
/// @param data 
/// @param size 
/// @return 
size_t FATFile::Write(const uint8_t* data, size_t size){
    return 0;
}
