#include "BiosDisk.hpp"
#include "RealMemory.hpp"
#include <core/Assert.hpp>
#include <core/cpp/Algorithm.hpp>
#include <core/cpp/Memory.hpp>

#define MODULE_NAME "BIOSDisk"

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

BIOSDisk::BIOSDisk(uint8_t id)
    : id(id),
      position(-1),
      size(0)
{
}

bool BIOSDisk::Initialize()
{
    haveExtensions = BIOSDiskExtensionPresent(id);

    if (haveExtensions)
    {
        ExtendedDriveParameters params;
        params.ParametersSize = sizeof(ExtendedDriveParameters);
        if (!BIOSDiskExtendedGetDriveParams(id, &params))
            return false;

        Assert(params.BytesPerSectors == SECTOR_SIZE);
        size = SECTOR_SIZE * params.Sectors;
    }
    else
    {
        uint8_t driveType;
        if (!BIOSDiskGetDriveParams(id, &driveType, &cylinders, &sectors, &heads))
            return false;
    }

    return true;
}

size_t BIOSDisk::Read(uint8_t* data, size_t size)
{
    size_t initialPos = position;
    if (position == -1)
    {
        ReadNextSector();
        position = 0;
    }
    if (position >= size)
    {
        return 0;
    }

    while (size > 0)
    {
        size_t bufferPos = position % SECTOR_SIZE;
        size_t canRead = min(size, SECTOR_SIZE - bufferPos);
        Memory::Copy(data, buffer + bufferPos, canRead);

        size -= canRead;
        data += canRead;
        position += canRead;

        if (size > 0)
        {
            ReadNextSector();
        }
    }

    return position - initialPos;
}

bool BIOSDisk::ReadNextSector()
{
    uint64_t lba = position / SECTOR_SIZE;
    bool ok = false;

    if (haveExtensions)
    {
        ExtendedReadParameters params;
        params.ParametersSize = sizeof(ExtendedDriveParameters);
        params.Reserved = 0;
        params.Count = 1;
        params.TransferBuffer = to_SegmentOffset(buffer);
        params.LBA = lba;

        for (int i = 0; i < 3 && !ok; i++) 
        {
            ok = BIOSDiskExtendedRead(id, &params);
            if (!ok)
                BIOSDiskReset(id);
        }
    }
    else
    {
        uint16_t cylinder, sector, head;
        LBA2CHS(lba, &cylinder, &sector, &head);

        for (int i = 0; i < 3 && !ok; i++) 
        {
            ok = BIOSDiskRead(id, cylinder, sector, head, 1, buffer);
            if (!ok)
                BIOSDiskReset(id);
        }
    }

    return ok;
}

size_t BIOSDisk::Write(const uint8_t* data, size_t size)
{
    return 0;
}

void BIOSDisk::LBA2CHS(uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / sectors) / heads;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / sectors) % heads;

}

bool BIOSDisk::Seek(SeekPos pos, int rel)
{
    switch (pos)
    {
        case SeekPos::Set:
            position = -1;
            return true;

        case SeekPos::Current:
            position += rel;
            ReadNextSector();
            return true;

        case SeekPos::End:
            position = size;
            return true;
    }

    return false;
}

size_t BIOSDisk::Size()
{
    return size;
}

size_t BIOSDisk::Position()
{
    return position;
}