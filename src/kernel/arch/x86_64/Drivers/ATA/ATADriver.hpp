#pragma once
#include <Serial.hpp>
class ATADriver
{
public:
    // Status Register Bits
    enum Status
    {
        BSY = 0x80,  // Busy
        DRDY = 0x40, // Drive Ready
        DF = 0x20,   // Drive Fault
        DRQ = 0x08,  // Data Request (ready to transfer)
        ERR = 0x01   // Error
    };

    static void WaitBusy()
    {
        // Wait for BSY to clear
        while (inb(0x1F7) & BSY)
            ;
    }

    static void WaitReady()
    {
        // Wait for DRQ to set
        while (!(inb(0x1F7) & DRQ))
            ;
    }

    static bool Identify()
    {
        // Select Drive (0xA0 for Master)
        outb(0x1F6, 0xA0);

        // Set sector count and LBA ports to 0
        outb(0x1F2, 0);
        outb(0x1F3, 0);
        outb(0x1F4, 0);
        outb(0x1F5, 0);

        // Send IDENTIFY command (0xEC)
        outb(0x1F7, 0xEC);

        // Check if drive exists (status 0 means no drive)
        uint8_t status = inb(0x1F7);
        if (status == 0)
            return false;

        WaitBusy();

        // Check for error (if LBA Mid/High are non-zero, it's NOT an ATA drive)
        if (inb(0x1F4) != 0 || inb(0x1F5) != 0)
            return false;

        WaitReady();

        // Read 256 words (512 bytes) of identification data
        uint16_t data[256];
        for (int i = 0; i < 256; i++)
        {
            data[i] = inw(0x1F0);
        }

        // Example: Sector 60-61 contains total LBA28 sectors
        uint32_t sectors = *((uint32_t *)&data[60]);

        return true;
    }

    static void ReadSector(uint32_t lba, uint8_t *buffer)
    {
        WaitBusy();

        outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // LBA mode + high 4 bits
        outb(0x1F2, 1);                           // Read 1 sector
        outb(0x1F3, (uint8_t)lba);                // LBA Low
        outb(0x1F4, (uint8_t)(lba >> 8));         // LBA Mid
        outb(0x1F5, (uint8_t)(lba >> 16));        // LBA High
        outb(0x1F7, 0x20);                        // Command: READ SECTORS

        WaitBusy();
        WaitReady();

        uint16_t *ptr = (uint16_t *)buffer;
        for (int i = 0; i < 256; i++)
        {
            ptr[i] = inw(0x1F0);
        }
    }
};