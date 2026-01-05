#pragma once
#include <stdint.h>
#include <arch/x86_64/Serial.hpp>
#include <cpp/String.hpp>
#include <globals.hpp>

struct GPTHeader
{
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    char disk_guid[16];
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
} __attribute__((packed));

struct GPTPartitionEntry
{
    char partition_type_guid[16];
    char unique_partition_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    char partition_name[72];
} __attribute__((packed));

class DiskDevice
{
public:
    static const uint16_t DATA_PORT = 0x1F0;
    static const uint16_t ERROR_PORT = 0x1F1;
    static const uint16_t SECTOR_COUNT_PORT = 0x1F2;
    static const uint16_t LBA_LOW_PORT = 0x1F3;
    static const uint16_t LBA_MID_PORT = 0x1F4;
    static const uint16_t LBA_HIGH_PORT = 0x1F5;
    static const uint16_t DRIVE_PORT = 0x1F6;
    static const uint16_t STATUS_PORT = 0x1F7;
    static const uint16_t COMMAND_PORT = 0x1F7;

    static const uint8_t CMD_READ_SECTORS = 0x20;
    static const uint8_t STATUS_BSY = 0x80;
    static const uint8_t STATUS_DRQ = 0x08;
    static const uint8_t STATUS_ERR = 0x01;

    static bool Initialize()
    {
        g_serialWriter->Print("Disk: Initializing IDE controller...\n");

        // Software reset
        outb(0x3F6, 0x04); // Set SRST bit
        for (volatile int i = 0; i < 10000; i++)
            ;              // Wait
        outb(0x3F6, 0x00); // Clear SRST bit

        // Wait for drive to be ready
        if (!WaitReady(5000))
        {
            g_serialWriter->Print("Disk: Drive not ready after reset\n");
            return false;
        }

        // Select master drive (drive 0)
        outb(DRIVE_PORT, 0xE0);
        for (volatile int i = 0; i < 1000; i++)
            ;

        // Check if drive exists
        uint8_t status = inb(STATUS_PORT);
        if (status == 0xFF)
        {
            g_serialWriter->Print("Disk: No drive detected\n");
            return false;
        }

        g_serialWriter->Print("Disk: IDE controller initialized\n");
        return true;
    }

    static bool ReadSectors(uint64_t lba, uint8_t sector_count, void *buffer)
    {
        if (sector_count == 0)
            return false;

        asm volatile("cli");

        if (!WaitReady(1000))
        {
            g_serialWriter->Print("Disk: Not ready\n");
            asm volatile("sti");
            return false;
        }

        // 1. Send the bits to the ports
        outb(SECTOR_COUNT_PORT, sector_count);
        outb(LBA_LOW_PORT, (uint8_t)(lba & 0xFF));
        outb(LBA_MID_PORT, (uint8_t)((lba >> 8) & 0xFF));
        outb(LBA_HIGH_PORT, (uint8_t)((lba >> 16) & 0xFF));

        // 0xE0 enables LBA mode. bits 24-27 of LBA go into the drive port.
        outb(DRIVE_PORT, 0xE0 | ((lba >> 24) & 0x0F));

        // 2. Send the Read command
        outb(COMMAND_PORT, CMD_READ_SECTORS);

        // 3. Read the data
        uint16_t *ptr = (uint16_t *)buffer;
        for (int sector = 0; sector < sector_count; sector++)
        {
            // Important: Wait for BSY to clear and DRQ to set for EVERY sector
            if (!WaitReady(1000))
            {
                g_serialWriter->Print("Disk: Read timeout\n");
                asm volatile("sti");
                return false;
            }

            for (int i = 0; i < 256; i++)
            {
                ptr[sector * 256 + i] = inw(DATA_PORT);
            }
        }

        asm volatile("sti");
        return true;
    }

private:
    static bool WaitReady(uint32_t timeout_ms = 1000)
    {
        uint32_t iterations = timeout_ms * 1000;
        while (iterations > 0)
        {
            uint8_t status = inb(STATUS_PORT);
            if (status & STATUS_ERR)
                return false;
            if (!(status & STATUS_BSY))
            {
                if (status & STATUS_DRQ)
                    return true;
                return true;
            }
            iterations--;
            for (volatile int i = 0; i < 100; i++)
                ;
        }
        return false;
    }
};

class GPTManager
{
public:
    struct PartitionInfo
    {
        uint64_t start_lba;
        uint64_t sector_count;
        bool found;
    };

    static PartitionInfo GetPartition(int index)
    {
        g_serialWriter->Print("GPT: Reading partition table...\n");

        uint8_t buffer[512];

        // Read GPT Header at LBA 1
        if (!DiskDevice::ReadSectors(1, 1, buffer))
        {
            g_serialWriter->Print("GPT: Failed to read header\n");
            return {0, 0, false};
        }

        GPTHeader *header = (GPTHeader *)buffer;

        // Verify GPT signature
        if (memcmp(header->signature, "EFI PART", 8) != 0)
        {
            g_serialWriter->Print("GPT: Invalid signature\n");
            return {0, 0, false};
        }

        g_serialWriter->Print("GPT: Valid header found\n");

        uint64_t entry_lba = header->partition_entries_lba;
        uint32_t entry_size = header->partition_entry_size;

        if (entry_size == 0 || entry_size > 512)
        {
            g_serialWriter->Print("GPT: Invalid entry size\n");
            return {0, 0, false};
        }

        // Read partition entries (typically at LBA 2)
        if (!DiskDevice::ReadSectors(entry_lba, 1, buffer))
        {
            g_serialWriter->Print("GPT: Failed to read entries\n");
            return {0, 0, false};
        }

        GPTPartitionEntry *entry = (GPTPartitionEntry *)(buffer + (index * entry_size));

        if (entry->starting_lba == 0)
        {
            g_serialWriter->Print("GPT: Partition not found at index\n");
            return {0, 0, false};
        }

        g_serialWriter->Print("GPT: Partition found at LBA: ");
        char buf[32];
        g_serialWriter->Print(itoa(entry->starting_lba, buf, 10));
        g_serialWriter->Print("\n");

        return {
            entry->starting_lba,
            (entry->ending_lba - entry->starting_lba),
            true};
    }
};