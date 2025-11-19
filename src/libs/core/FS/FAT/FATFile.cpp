#include "FATFile.hpp"
#include <FS/FATFileSystem.hpp>
#include <core/Debug.hpp>
#include <core/cpp/Algorithm.hpp>
#include <core/cpp/Memory.hpp>

#define LOG_MODULE "FatFile"

FATFile::FATFile()
    : fs(nullptr),
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

bool FATFile::Open(FATFileSystem* filesystem, uint32_t firstCluster, const char* name, uint32_t size, bool isDirectory)
{

    Debug::Info(LOG_MODULE, " Regular Open -> first cluster: %lu name: %s size: %lu isDirectory: %d", firstCluster, name, size, isDirectory);

    isRootDirectory = false;
    position = 0;
    this->size = size;
    this->isDirectory = isDirectory;
    FirstCluster = firstCluster;
    CurrentCluster = FirstCluster;
    CurrentClusterIndex = 0;
    CurrentSectorInCluster = 0;

    if (!fs->ReadSectorFromCluster(CurrentCluster, CurrentSectorInCluster, Buffer))
    {
        Debug::Error(LOG_MODULE, "FAT: open file %s failed - read error cluster=%u\n", name, CurrentCluster);
        return false;
    }

    Opened = true;
    return true;
}

bool FATFile::OpenFat1216RootDirectory(FATFileSystem* filesystem, uint32_t rootDirLba, uint32_t rootDirSize)
{
    isRootDirectory = true;
    position = 0;
    size = rootDirSize;
    FirstCluster = rootDirLba;
    CurrentCluster = FirstCluster;
    CurrentClusterIndex = 0;
    CurrentSectorInCluster = 0;

    if (!fs->ReadSector(rootDirLba, Buffer))
    {
        Debug::Error(LOG_MODULE, "FAT: read root directory failed\r\n");
        return false;
    }

    return true;
}

void FATFile::Release()
{
    fs->ReleaseFile(this);
}

bool FATFile::ReadFileEntry(FATDirectoryEntry* dirEntry)
{
    return Read(reinterpret_cast<uint8_t*>(dirEntry), sizeof(FATDirectoryEntry)) == sizeof(FATDirectoryEntry);
}
size_t FATFile::Read(uint8_t* data, size_t byteCount)
{
    uint8_t* originalData = data;

    // don't read past the end of the file
    if (!isDirectory || (isDirectory && size != 0))
        byteCount = min(byteCount, size - position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SectorSize - (position % SectorSize);
        uint32_t take = min(byteCount, leftInBuffer);

        Memory::Copy(data, Buffer + position % SectorSize, take);
        data += take;
        position += take;
        byteCount -= take;

        // printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // Special handling for root directory
            if (isRootDirectory)
            {
                ++CurrentCluster;

                // read next sector
                if (!fs->ReadSector(CurrentCluster, Buffer))
                {
                    Debug::Error(LOG_MODULE, "FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                // calculate next cluster & sector to read
                if (++CurrentSectorInCluster >= fs->Data().BS.BootSector.SectorsPerCluster)
                {
                    CurrentSectorInCluster = 0;
                    CurrentCluster = fs->GetNextCluster(CurrentCluster);
                    ++CurrentClusterIndex;
                }

                if (CurrentCluster >= 0xFFFFFFF8)
                {
                    // Mark end of file
                    size = position;
                    break;
                }

                // read next sector
                if (!fs->ReadSectorFromCluster(CurrentCluster, CurrentSectorInCluster, Buffer))
                {
                    Debug::Error(LOG_MODULE, "FAT: read error!");
                    break;
                }
            }
        }
    }

    return data - originalData;
}



size_t FATFile::Write(const uint8_t* data, size_t size)
{
    // not supported (yet)
    return 0;
}

bool FATFile::Seek(SeekPos pos, int rel)
{
    switch (pos)
    {
    case SeekPos::Set:
        position = static_cast<uint32_t>(max(0, rel));
        break;
    
    case SeekPos::Current:
        if (rel < 0 && position < -rel)
            position = 0;

        position = min(size, static_cast<uint32_t>(position + rel));
        break;
    case SeekPos::End:
        if (rel < 0 && size < -rel)
            position = 0;
        position = min(size, static_cast<uint32_t>(Size() + rel));

        break;
    }

    UpdateCurrentCluster();
    return true;
}

bool FATFile::UpdateCurrentCluster()
{
    uint32_t clusterSize = fs->Data().BS.BootSector.SectorsPerCluster * SectorSize;
    uint32_t desiredCluster = position / clusterSize;
    uint32_t desiredSector = (position % clusterSize) / SectorSize;
    
    if (desiredCluster == CurrentClusterIndex && desiredSector == CurrentSectorInCluster)
        return true;

    if (desiredCluster < CurrentClusterIndex)
    {
        CurrentClusterIndex = 0;
        CurrentCluster = FirstCluster;
    }

    while (desiredCluster > CurrentClusterIndex) 
    {
        CurrentCluster = fs->GetNextCluster(CurrentCluster);
        ++CurrentClusterIndex;
    }

    CurrentSectorInCluster = desiredSector;
    return fs->ReadSectorFromCluster(CurrentCluster, CurrentSectorInCluster, Buffer);
}

FileEntry* FATFile::ReadFileEntry()
{
    FATDirectoryEntry entry;
    if (ReadFileEntry(&entry))
    {
        FATFileEntry* fileEntry = fs->AllocateFileEntry();
        fileEntry->Initialize(fs, entry);
        return fileEntry;
    }

    return nullptr;
}