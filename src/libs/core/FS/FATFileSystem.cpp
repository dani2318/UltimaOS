#include "FATFileSystem.hpp"
#include <Debug.hpp>

constexpr const char* module_name = "FAT";

constexpr int ROOT_DIRECTORY_HANDLE = -1;

FATFileSystem::FATFileSystem()
    : device(), 
      Data(new FATData()),
      DataSectionLba(),
      FatType(),
      TotalSectors(),
      SectorsPerFat() {}

void FATFileSystem::Detect() {
    uint32_t dataCluster = (TotalSectors - DataSectionLba) / Data->BS.BootSector.SectorsPerCluster;
    if (dataCluster < 0xFF5) {
        FatType = FAT12;
    } else if (Data->BS.BootSector.SectorsPerFat != 0) {
        FatType = FAT16;
    } else{
        FatType = FAT32;
    }
}

uint32_t FATFileSystem::ClusterToLba(uint32_t cluster)
{
    return DataSectionLba + (cluster - 2) * Data->BS.BootSector.SectorsPerCluster;
}

bool FATFileSystem::ReadSectorFromCluster(uint32_t cluster, uint32_t sectorOffset, uint8_t* buffer){
    return ReadSector(this->ClusterToLba(cluster) + sectorOffset, buffer);
}

bool FATFileSystem::ReadBootSector(){
    return ReadSector(0, Data->BS.BootSectorBytes);
}

bool FATFileSystem::ReadSector(uint32_t lba, uint8_t* buffer, size_t count){
    this->device->Seek(SeekPos::Set, lba*SectorSize);
    return this->device->Read(buffer, count * SectorSize) == count *SectorSize;
}

bool FATFileSystem::Initialize(BlockDevice* device) {
    this->device = device;

    // Trying to read the bootsector.
    if(!ReadBootSector()){
        Debug::Error(module_name,"Failed to read bootsector! ");
        return false;
    }

    // Setting FatCachePosition to 0xFFFFFFFF (this number will change fater the Initialization and the Filesystem started to read files).
    Data->FatCachePosition = 0xFFFFFFFF;

    Detect();

    // FatType is set by the call to the 'Detect()' function before if is equal to FATType::FAT32 then the file system is using FAT32 and the boolean is true otherwise is false
    bool isFAT32 = FatType == FATType::FAT32;

    // If we are using FAT32 and should use 'g_Data->BS.BootSector.EBR32.SectorsPerFat' as SectorPerFat
    // and should use 'g_Data->BS.BootSector.LargeSectorCount' as TotalSectors

    if(isFAT32){
        TotalSectors = Data->BS.BootSector.LargeSectorCount;
        SectorsPerFat = Data->BS.BootSector.EBR32.SectorsPerFat;
    }else{  
        // In this case we are in FAT12 or FAT16
        TotalSectors = Data->BS.BootSector.TotalSectors;
        SectorsPerFat = Data->BS.BootSector.SectorsPerFat;
    }
    
    // Reading from the file system the Directory Entry of the Root Directory.
    if (isFAT32) {
        DataSectionLba = Data->BS.BootSector.ReservedSectors + SectorsPerFat * Data->BS.BootSector.FatCount;
        if(!Data->RootDirectory.Open(this, Data->BS.BootSector.EBR32.RootdirCluster,"", 0xFFFFFFFF, true))
            return false;
    } else {
        uint32_t rootDirLBA = Data->BS.BootSector.ReservedSectors + SectorsPerFat * Data->BS.BootSector.FatCount;
        uint32_t rootDirSize = sizeof(FATDirectoryEntry) * Data->BS.BootSector.DirEntryCount;
        uint32_t rootDirectorySectors = (rootDirSize + Data->BS.BootSector.BytesPerSector - 1) / Data->BS.BootSector.BytesPerSector;
        DataSectionLba = rootDirLBA + rootDirectorySectors;
        if(!Data->RootDirectory.OpenFat1216RootDirectory(this, rootDirLBA, rootDirSize))
            return false;
    }

    // Long file name is 0 by default!
    Data->LFNCount = 0;

    return true;
}

FATFile* FATFileSystem::AllocateFile(){
    return Data->OpenedFilePool.Allocate();
}

void FATFileSystem::ReleaseFile(FATFile* file){
    Data->OpenedFilePool.Free(file);
}

FATFileEntry* FATFileSystem::AllocateFileEntry(){
    return Data->FileEntryPool.Allocate();
}

void FATFileSystem::ReleaseFileEntry(FATFileEntry* file){
    Data->FileEntryPool.Free(file);
}

File* FATFileSystem::RootDirectory(){
    return &Data->RootDirectory;
}


uint32_t FATFileSystem::GetNextCluster(uint32_t currentCluster){

    // Determine the offset of the entry to read.
    uint32_t fatIndex;
    if (FatType == FAT12) {
        fatIndex = currentCluster * 3 / 2;
    } else if(FatType == FAT16) {
        fatIndex = currentCluster * 2;
    } else /* if (g_FatType == 32)*/ {
        fatIndex = currentCluster * 4;
    }

    // Check if cache has correct number

    uint32_t fatIndexSector = fatIndex / SectorSize;

    if( fatIndexSector < Data->FatCachePosition
        || fatIndexSector >= Data->FatCachePosition + SectorSize
    ){
        ReadFat(fatIndexSector);
        Data->FatCachePosition = fatIndexSector;
    }

    fatIndex -= (Data->FatCachePosition * SectorSize);

    uint32_t nextCluster;
    if (FatType == FAT12) {
        if(currentCluster % 2 == 0){
            nextCluster = (*(uint16_t *)(Data->FatCache + fatIndex)) & 0x0FFF;
        }else {
            nextCluster = (*(uint16_t *)(Data->FatCache + fatIndex)) >> 4;
        }

        if (nextCluster >= 0xFF8) {
            nextCluster |= 0xFFFFF000;
        }

    } else if(FatType == FAT16) {
        nextCluster = *(uint16_t *)(Data->FatCache + fatIndex);

        if (nextCluster >= 0xFFF8) {
            nextCluster |= 0xFFFF0000;
        }

    } else /*if (g_FatType == 32)*/ {
        nextCluster = *(uint32_t *)(Data->FatCache + fatIndex);
    }

    return nextCluster;
}


bool FATFileSystem::ReadFat(uint32_t lbaOffset)
{
    return ReadSector(
        Data->BS.BootSector.ReservedSectors + lbaOffset,
        Data->FatCache,
        FatCacheSize
    );
}