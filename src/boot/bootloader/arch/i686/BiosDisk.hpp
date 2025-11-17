#pragma once
#include <core/dev/BlockDevice.hpp>
#include <core/Defs.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "x86.h"

static inline const constexpr int SECTOR_SIZE = 512;

class BIOSDisk : public BlockDevice {
    public:

        BIOSDisk(uint8_t deviceID);
        bool Initialize();
        size_t Write(const uint8_t* data, size_t size) override; 
        size_t Read(uint8_t* data, size_t size) override; 
        bool Seek(SeekPos pos, int rel) override;
        size_t Size() override;
        size_t Position() override;

    private:
        bool ReadNextSector();
        void LBA2CHS(uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut);
        uint8_t  id;
        uint16_t cylinders;
        uint16_t sectors;
        uint16_t heads;
        bool haveExtensions;
        uint8_t buffer[SECTOR_SIZE];
        uint64_t position;
        uint64_t size;
};