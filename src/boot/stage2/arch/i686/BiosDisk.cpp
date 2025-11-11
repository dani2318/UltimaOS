#include "BiosDisk.hpp"
#include "RealMemory.hpp"
#include <core/Assert.hpp>
#include <core/cpp/Algorithm.hpp>
#include <core/cpp/Memory.hpp>

EXPORT bool ASMCALL BIOSDiskGetDriveParams(uint8_t drive,
                                           uint8_t *driveTypeOut,
                                           uint16_t *cylindersOut,
                                           uint16_t *sectorsOut,
                                           uint16_t *headsOut);

EXPORT bool ASMCALL BIOSDiskReset(uint8_t drive);

EXPORT bool ASMCALL BIOSDiskRead(uint8_t drive,
                                 uint16_t cylinder,
                                 uint16_t head,
                                 uint16_t sector,
                                 uint8_t count,
                                 uint8_t *dataOut);

EXPORT bool ASMCALL BIOSDiskExtensionPresent(uint8_t drive);

struct ExtendedDriveParameters
{
    uint16_t ParametersSize;
    uint16_t Flags;
    uint32_t Cylinders;
    uint32_t Heads;
    uint32_t SectorsPerTrack;
    uint64_t Sectors;
    uint16_t BytesPerSectors;
} __attribute__((packed));

EXPORT bool ASMCALL BIOSDiskExtendedGetDriveParams(uint8_t drive,
                                                   ExtendedDriveParameters *params);

struct ExtendedReadParameters
{
    uint8_t ParametersSize;
    uint8_t Reserved;
    uint16_t Count;
    uint32_t TransferBuffer;
    uint64_t LBA;
} __attribute__((packed));

EXPORT bool ASMCALL BIOSDiskExtendedRead(uint8_t drive,
                                         ExtendedReadParameters *params);

BIOSDisk::BIOSDisk(uint8_t deviceID)
    : id(deviceID),
      position(-1),
      size(0)
{
}

bool BIOSDisk::Initialize()
{
    Debug::Info("Stage2", "Checking extensions...");
    haveExtensions = BIOSDiskExtensionPresent(id);
    
    if (haveExtensions)
    {
        Debug::Info("Stage2", "Extensions present, getting params...");
        ExtendedDriveParameters parameters;
        parameters.ParametersSize = sizeof(ExtendedDriveParameters);
        if (!BIOSDiskExtendedGetDriveParams(id, &parameters))
        {
            Debug::Info("Stage2", "Failed to get extended params");
            return false;
        }
        Debug::Info("Stage2", "Got extended params");
        Assert(parameters.BytesPerSectors == SECTOR_SIZE);
        this->size = SECTOR_SIZE * parameters.Sectors;
    }
    else
    {
        Debug::Info("Stage2", "No extensions, using legacy method...");
        uint8_t driveType;
        if (!BIOSDiskGetDriveParams(id, &driveType, &cylinders, &sectors, &heads))
        {
            Debug::Info("Stage2", "Failed to get legacy params");
            return false;
        }
        Debug::Info("Stage2", "Got legacy params");
    }
    
    Debug::Info("Stage2", "Initialize complete!");
    return true;
}
size_t BIOSDisk::Write(const uint8_t *data, size_t size)
{
    return 0;
}

void BIOSDisk::LBA2CHS(uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut){
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / sectors) / heads;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / sectors) % heads;
}


bool BIOSDisk::ReadNextSector(){

    uint64_t lba = position / SECTOR_SIZE;
    bool success = false;

    if(haveExtensions){
        ExtendedReadParameters readParameters;
        readParameters.ParametersSize = sizeof(ExtendedDriveParameters);
        readParameters.Reserved = 0;
        readParameters.Count = 1;
        readParameters.TransferBuffer = to_SegmentOffset(buffer);
        readParameters.LBA = lba;

        for (int i = 0; i < 3 && !success; i++){
            success = success & BIOSDiskExtendedRead(id, &readParameters);
            if(!success)
                BIOSDiskReset(id);
        }

    } else {
        uint16_t cylinder, sector, head;

        LBA2CHS(lba, &cylinder, &sector, &head);
        
        for (int i = 0; i < 3 && !success; i++){
            success = success & BIOSDiskRead(id, cylinder, sector, head, 1, buffer);
            if(!success)
                BIOSDiskReset(id);
        }
    }


    return success;

}


size_t BIOSDisk::Read(uint8_t *data, size_t size)
{   
    uint64_t initialposition = position;
    if(position == -1){
        ReadNextSector();
        position = 0;
    }
    if(position >= size)
        return 0;
    
    while( size > 0){
        size_t bufferpos = position % SECTOR_SIZE;
        size_t leftInBuffer = SECTOR_SIZE - bufferpos;
        size_t canRead = min(size, leftInBuffer);
        Memory::Copy(data, buffer + bufferpos, canRead);
        size -= canRead;
        data += canRead;
        position += canRead;

        if(size > 0){
            ReadNextSector();
        }
    }

    return position - initialposition;

}

bool BIOSDisk::Seek(SeekPos pos, int rel)
{
    switch(pos){
        case SeekPos::Set:
            position = -1;
            break;
        case SeekPos::Current:
            position += rel;
            ReadNextSector();
            break;
        case SeekPos::End:
            position = size;
            break;
    }

    return true;
}

size_t BIOSDisk::Size()
{
    return this->size;
}

size_t BIOSDisk::Position() {
    return this->position;
}
