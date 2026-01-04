#pragma once
#include <stdint.h>
#include <arch/x86_64/Filesystem/GPT.hpp>
#include <globals.hpp>
struct FAT32_BPB
{
    uint8_t jmp[3];
    char oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries_count; // 0 for FAT32
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 Extended fields
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char system_identifier[8];
} __attribute__((packed));

struct FAT_DirectoryEntry
{
    char name[11];      // 8.3 filename format
    uint8_t attributes; // 0x10 = Directory, 0x20 = Archive
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high; // High 16 bits of cluster
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low; // Low 16 bits of cluster
    uint32_t file_size;
} __attribute__((packed));

class FAT32Filesystem
{
private:
    uint64_t partition_start_lba;
    FAT32_BPB bpb;

public:
    FAT32Filesystem(uint64_t start_lba) : partition_start_lba(start_lba)
    {
        DiskDevice::ReadSectors(start_lba, 1, &bpb);
    }

    uint64_t ClusterToLBA(uint32_t cluster)
    {
        uint32_t fat_size = bpb.sectors_per_fat_32;
        uint32_t data_dir_lba = bpb.reserved_sectors + (bpb.fat_count * fat_size);
        return partition_start_lba + data_dir_lba + (cluster - 2) * bpb.sectors_per_cluster;
    }

    void *ReadFile(const char *filename, size_t *out_size)
    {
        // 1. Convert "SHELL.ELF" to "SHELL   ELF" (8.3 format)
        char fatName[11];
        memset(fatName, ' ', 11);
        int i = 0, j = 0;
        while (filename[i] != '.' && filename[i] != 0 && j < 8)
            fatName[j++] = filename[i++];
        if (filename[i] == '.')
        {
            i++;
            j = 8;
            while (filename[i] != 0 && j < 11)
                fatName[j++] = filename[i++];
        }
        // Convert to uppercase
        for (int k = 0; k < 11; k++)
            if (fatName[k] >= 'a' && fatName[k] <= 'z')
                fatName[k] -= 32;

        // 2. Read Root Directory (Starting at bpb.root_cluster)
        uint32_t clusterSize = bpb.sectors_per_cluster * 512;
        uint8_t *buffer = (uint8_t *)g_heap.Alloc(clusterSize);

        // We start searching at the root cluster
        DiskDevice::ReadSectors(ClusterToLBA(bpb.root_cluster), bpb.sectors_per_cluster, buffer);

        FAT_DirectoryEntry *dir = (FAT_DirectoryEntry *)buffer;
        for (uint32_t i = 0; i < clusterSize / sizeof(FAT_DirectoryEntry); i++)
        {
            if (dir[i].name[0] == 0)
                break; // End of dir

            if (memcmp(dir[i].name, fatName, 11) == 0)
            {
                uint32_t firstCluster = ((uint32_t)dir[i].first_cluster_high << 16) | dir[i].first_cluster_low;
                uint32_t fileSize = dir[i].file_size;

                // 3. Allocate final buffer and read file data
                void *fileData = g_heap.Alloc(fileSize);
                uint32_t sectorsNeeded = (fileSize + 511) / 512;

                DiskDevice::ReadSectors(ClusterToLBA(firstCluster), sectorsNeeded, fileData);

                *out_size = fileSize;
                g_heap.Free(buffer);
                return fileData;
            }
        }

        g_heap.Free(buffer);
        return nullptr;
    }
};