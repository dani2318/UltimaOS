#pragma once
#include <stdint.h>
#include <arch/x86_64/Serial.hpp>

struct GPTHeader {
    char signature[8]; // "EFI PART"
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    char disk_guid[16];
    uint64_t partition_entries_lba; // Usually LBA 2
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
} __attribute__((packed));

struct GPTPartitionEntry {
    char partition_type_guid[16];
    char unique_partition_guid[16];
    uint64_t starting_lba; // <--- THIS is what you need for FAT32
    uint64_t ending_lba;
    uint64_t attributes;
    char partition_name[72];
} __attribute__((packed));

class DiskDevice {
public:
    static const uint16_t DATA_PORT = 0x1F0;
    static const uint16_t STATUS_PORT = 0x1F7;
    static const uint16_t COMMAND_PORT = 0x1F7;

    static void ReadSectors(uint64_t lba, uint8_t sector_count, void* buffer) {
        WaitReady();

        // LBA28 Setup
        outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
        outb(0x1F2, sector_count);
        outb(0x1F3, (uint8_t)lba);
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F5, (uint8_t)(lba >> 16));
        outb(0x1F7, 0x20); // Read Command

        uint16_t* ptr = (uint16_t*)buffer;
        for (int j = 0; j < sector_count; j++) {
            WaitReady();
            for (int i = 0; i < 256; i++) {
                *ptr++ = inw(DATA_PORT);
            }
        }
    }

private:
    static void WaitReady() {
        while ((inb(STATUS_PORT) & 0x80)); // Wait for BSY to clear
        while (!(inb(STATUS_PORT) & 0x08)); // Wait for DRQ to set
    }
};

class GPTManager {
public:
    struct PartitionInfo {
        uint64_t start_lba;
        uint64_t sector_count;
        bool found;
    };

    static PartitionInfo GetPartition(int index) {
        uint8_t buffer[512];
        DiskDevice::ReadSectors(1, 1, buffer); // Read GPT Header
        
        GPTHeader* header = (GPTHeader*)buffer;
        uint64_t entry_lba = header->partition_entries_lba;
        uint32_t entry_size = header->partition_entry_size;

        // Read first sector of entries
        DiskDevice::ReadSectors(entry_lba, 1, buffer);
        GPTPartitionEntry* entry = (GPTPartitionEntry*)(buffer + (index * entry_size));

        if (entry->starting_lba == 0) return {0, 0, false};
        return {entry->starting_lba, (entry->ending_lba - entry->starting_lba), true};
    }
};