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
        g_serialWriter->Print("FAT32: Initializing at LBA ");
        char b[64];
        g_serialWriter->Print(itoa(start_lba, b, 10));
        g_serialWriter->Print("\n");

        DiskDevice::ReadSectors(start_lba, 1, &bpb);

        g_serialWriter->Print("FAT32: Signature: ");
        g_serialWriter->PrintHex(bpb.boot_signature); // Should be 0x29
        g_serialWriter->Print("\n");

        g_serialWriter->Print("FAT32: OEM ID: ");
        g_serialWriter->Print(bpb.oem_id); // Usually "MSDOS5.0" or "mkfs.fat"
        g_serialWriter->Print("\n");
    }

    uint64_t ClusterToLBA(uint32_t cluster)
    {
        uint32_t fat_size = bpb.sectors_per_fat_32;
        uint32_t data_dir_lba = bpb.reserved_sectors + (bpb.fat_count * fat_size);
        return partition_start_lba + data_dir_lba + (cluster - 2) * bpb.sectors_per_cluster;
    }

    void ListRootDirectory()
    {
        uint32_t lba = ClusterToLBA(bpb.root_cluster);
        char outBuf[64];

        g_serialWriter->Print("Reading Root Dir at LBA: ");
        g_serialWriter->Print(itoa(lba, outBuf, 10));
        g_serialWriter->Print("\n");

        uint8_t sectorData[512];
        if (!DiskDevice::ReadSectors(lba, 1, sectorData))
        {
            g_serialWriter->Print("ERROR: Could not read Root LBA\n");
            return;
        }

        // 1. Check if the sector is actually empty
        bool allZero = true;
        for (int i = 0; i < 16; i++)
        {
            if (sectorData[i] != 0)
                allZero = false;
            g_serialWriter->PrintHex(sectorData[i]);
            g_serialWriter->Print(" ");
        }
        g_serialWriter->Print("\n");

        if (allZero)
        {
            g_serialWriter->Print("FATAL: Sector is empty. Disk image might be blank at this LBA.\n");
            return;
        }

        // 2. Iterate entries
        FAT_DirectoryEntry *dir = (FAT_DirectoryEntry *)sectorData;
        g_serialWriter->Print("--- Root Directory Listing ---\n");
        for (int i = 0; i < 512 / sizeof(FAT_DirectoryEntry); i++)
        {
            if (dir[i].name[0] == 0x00)
                break;
            if ((uint8_t)dir[i].name[0] == 0xE5)
                continue;
            if (dir[i].attributes == 0x0F)
                continue; // Skip LFN

            g_serialWriter->Print("'");
            for (int k = 0; k < 11; k++)
            {
                char c[2] = {dir[i].name[k], 0};
                g_serialWriter->Print(c);
            }
            g_serialWriter->Print("' ");

            if (dir[i].attributes & 0x10)
                g_serialWriter->Print("[DIR]\n");
            else
                g_serialWriter->Print("[FILE]\n");
        }
    }

    void ToFatName(const char *input, char *output)
    {
        // Fill with spaces initially
        memset(output, ' ', 11);

        int i = 0;
        int j = 0;

        // Process the name part (up to 8 chars)
        while (input[i] != '\0' && input[i] != '.' && j < 8)
        {
            char c = input[i];
            // Convert to uppercase
            if (c >= 'a' && c <= 'z')
                c -= 32;
            output[j++] = c;
            i++;
        }

        // Process the extension part (if there is a dot)
        if (input[i] == '.')
        {
            i++;   // Skip the dot
            j = 8; // Jump to extension offset in FAT name
            while (input[i] != '\0' && j < 11)
            {
                char c = input[i];
                if (c >= 'a' && c <= 'z')
                    c -= 32;
                output[j++] = c;
                i++;
            }
        }
    }

    uint32_t GetNextCluster(uint32_t cluster)
    {
        uint32_t fat_offset = cluster * 4;
        // ERROR CHECK: Did you include the partition start?
        uint32_t fat_sector = partition_start_lba + bpb.reserved_sectors + (fat_offset / 512);
        uint32_t ent_offset = fat_offset % 512;

        uint8_t table[512];
        if (!DiskDevice::ReadSectors(fat_sector, 1, table))
            return 0x0FFFFFF7;

        // Read the entry and mask the top 4 bits (reserved in FAT32)
        uint32_t next_cluster = *(uint32_t *)&table[ent_offset] & 0x0FFFFFFF;
        return next_cluster;
    }

    void *ReadFile(const char *filepath, size_t *out_size)
    {
        g_serialWriter->Print("FAT32: Attempting to read: ");
        g_serialWriter->Print(filepath);
        g_serialWriter->Print("\n");

        // 1. Safety Copy: strtok is destructive. Do not parse the original pointer!
        char pathBuffer[256];
        size_t pathLen = strlen(filepath);
        if (pathLen >= 256)
            return nullptr;
        strcpy(pathBuffer, filepath);

        uint32_t currentCluster = bpb.root_cluster;
        FAT_DirectoryEntry entry;
        bool found = false;

        // 2. Traversal: Loop through each part of the path (e.g., "PROGRAMS", then "SHELL.ELF")
        char *part = strtok(pathBuffer, "/\\");
        while (part != nullptr)
        {
            char targetFatName[11];
            ToFatName(part, targetFatName); // Use the 8.3 formatting helper

            uint32_t clusterSize = bpb.sectors_per_cluster * 512;
            uint8_t *dirBuf = (uint8_t *)g_heap.Alloc(clusterSize);

            found = false;
            uint32_t searchCluster = currentCluster;

            // Search through the directory's cluster chain
            while (searchCluster < 0x0FFFFFF8)
            {
                if (!DiskDevice::ReadSectors(ClusterToLBA(searchCluster), bpb.sectors_per_cluster, dirBuf))
                    break;

                FAT_DirectoryEntry *dir = (FAT_DirectoryEntry *)dirBuf;
                for (uint32_t i = 0; i < clusterSize / sizeof(FAT_DirectoryEntry); i++)
                {
                    if (dir[i].name[0] == 0x00)
                        break; // End of directory
                    if (dir[i].name[0] == 0xE5 || dir[i].attributes == 0x0F)
                        continue;

                    if (memcmp(dir[i].name, targetFatName, 11) == 0)
                    {
                        entry = dir[i];
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                searchCluster = GetNextCluster(searchCluster); // Follow the chain
            }

            g_heap.Free(dirBuf);
            if (!found)
            {
                g_serialWriter->Print("FAT32: Path component not found: ");
                g_serialWriter->Print(part);
                g_serialWriter->Print("\n");
                return nullptr;
            }

            currentCluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
            part = strtok(nullptr, "/\\");
        }

        // 3. Data Loading: Follow the cluster chain to load the actual file
        *out_size = entry.file_size;
        uint8_t *fileData = (uint8_t *)g_heap.Alloc(entry.file_size);
        if (!fileData)
            return nullptr;

        uint32_t bytesRemaining = entry.file_size;
        uint32_t fileCluster = currentCluster;
        uint32_t clusterSize = bpb.sectors_per_cluster * 512;
        uint8_t *tempClusterBuf = (uint8_t *)g_heap.Alloc(clusterSize);

        while (fileCluster < 0x0FFFFFF8 && bytesRemaining > 0)
        {
            uint8_t *dest = fileData + (entry.file_size - bytesRemaining);

            char dbg_ptr[32];
            g_serialWriter->Print("Reading Cluster: ");
            g_serialWriter->Print(itoa(fileCluster, dbg_ptr, 10));
            g_serialWriter->Print(" to Buffer: ");
            g_serialWriter->PrintHex((uint64_t)dest);
            g_serialWriter->Print("\n");

            // Try commenting out ONLY the ReadSectors call.
            // If it stops rebooting, the IDE driver is the problem.
            if (!DiskDevice::ReadSectors(ClusterToLBA(fileCluster), bpb.sectors_per_cluster, tempClusterBuf))
            {
                return nullptr;
            }
            uint32_t toCopy = (bytesRemaining > clusterSize) ? clusterSize : bytesRemaining;
            memcpy(dest, tempClusterBuf, toCopy);

            bytesRemaining -= toCopy;
            fileCluster = GetNextCluster(fileCluster);
        }
        g_serialWriter->Print("\nFAT32: Data read complete. Returning to caller.\n");
        g_heap.Free(tempClusterBuf);
        g_serialWriter->Print("FAT32: File loaded successfully.\n");
        return fileData;
    }
};