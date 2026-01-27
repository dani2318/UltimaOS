#pragma once
#include <stdint.h>
#include <arch/x86_64/Filesystem/GPT.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
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

// FAT32 file attributes
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LONG_NAME  0x0F

// FAT32 cluster markers
#define FAT32_EOC_MIN       0x0FFFFFF8  // End of chain
#define FAT32_BAD_CLUSTER   0x0FFFFFF7  // Bad cluster
#define FAT32_FREE_CLUSTER  0x00000000  // Free cluster

class FAT32Filesystem
{
private:
    uint64_t partition_start_lba;
    FAT32_BPB bpb;
    bool initialized;

    // Cache for FAT table sector (reduces disk I/O)
    uint8_t fat_cache[512];
    uint32_t fat_cache_sector;
    bool fat_cache_valid;

public:
    FAT32Filesystem(uint64_t start_lba) 
        : partition_start_lba(start_lba), 
          initialized(false),
          fat_cache_sector(0xFFFFFFFF),
          fat_cache_valid(false)
    {
        g_serialWriter->Print("FAT32: Initializing at LBA ");
        char b[64];
        g_serialWriter->Print(itoa(start_lba, b, 10));
        g_serialWriter->Print("\n");

        // Read boot sector
        if (!DiskDevice::ReadSectors(start_lba, 1, &bpb)) {
            g_serialWriter->Print("FAT32: Failed to read boot sector\n");
            return;
        }

        // Validate FAT32 signature
        if (bpb.boot_signature != 0x28 && bpb.boot_signature != 0x29) {
            g_serialWriter->Print("FAT32: Invalid boot signature: 0x");
            g_serialWriter->PrintHex(bpb.boot_signature);
            g_serialWriter->Print(" (expected 0x28 or 0x29)\n");
            return;
        }

        // Validate basic parameters
        if (bpb.bytes_per_sector != 512) {
            g_serialWriter->Print("FAT32: Unsupported sector size: ");
            g_serialWriter->Print(itoa(bpb.bytes_per_sector, b, 10));
            g_serialWriter->Print(" (only 512 supported)\n");
            return;
        }

        if (bpb.sectors_per_cluster == 0 || bpb.sectors_per_cluster > 128) {
            g_serialWriter->Print("FAT32: Invalid sectors per cluster: ");
            g_serialWriter->Print(itoa(bpb.sectors_per_cluster, b, 10));
            g_serialWriter->Print("\n");
            return;
        }

        if (bpb.fat_count == 0) {
            g_serialWriter->Print("FAT32: Invalid FAT count: 0\n");
            return;
        }

        if (bpb.root_cluster < 2) {
            g_serialWriter->Print("FAT32: Invalid root cluster: ");
            g_serialWriter->Print(itoa(bpb.root_cluster, b, 10));
            g_serialWriter->Print("\n");
            return;
        }

        g_serialWriter->Print("FAT32: OEM ID: ");
        for (int i = 0; i < 8 && bpb.oem_id[i]; i++) {
            char c[2] = {bpb.oem_id[i], 0};
            g_serialWriter->Print(c);
        }
        g_serialWriter->Print("\n");

        g_serialWriter->Print("FAT32: Bytes per sector: ");
        g_serialWriter->Print(itoa(bpb.bytes_per_sector, b, 10));
        g_serialWriter->Print("\n");
        
        g_serialWriter->Print("FAT32: Sectors per cluster: ");
        g_serialWriter->Print(itoa(bpb.sectors_per_cluster, b, 10));
        g_serialWriter->Print("\n");
        
        g_serialWriter->Print("FAT32: Root cluster: ");
        g_serialWriter->Print(itoa(bpb.root_cluster, b, 10));
        g_serialWriter->Print("\n");

        initialized = true;
        g_serialWriter->Print("FAT32: Initialization complete\n");
    }

    bool IsInitialized() const { return initialized; }

    uint64_t ClusterToLBA(uint32_t cluster)
    {
        if (cluster < 2) {
            g_serialWriter->Print("FAT32: Invalid cluster number: ");
            char b[32];
            g_serialWriter->Print(itoa(cluster, b, 10));
            g_serialWriter->Print("\n");
            return 0;
        }

        uint32_t fat_size = bpb.sectors_per_fat_32;
        uint32_t data_start_lba = bpb.reserved_sectors + (bpb.fat_count * fat_size);
        return partition_start_lba + data_start_lba + ((uint64_t)(cluster - 2) * bpb.sectors_per_cluster);
    }

    void ListRootDirectory()
    {
        if (!initialized) {
            g_serialWriter->Print("FAT32: Not initialized\n");
            return;
        }

        uint32_t lba = ClusterToLBA(bpb.root_cluster);
        char outBuf[64];

        g_serialWriter->Print("FAT32: Reading Root Dir at LBA: ");
        g_serialWriter->Print(itoa(lba, outBuf, 10));
        g_serialWriter->Print("\n");

        uint8_t sectorData[512];
        
        // Yield to other tasks before I/O
        Scheduler::Yield();
        
        if (!DiskDevice::ReadSectors(lba, 1, sectorData))
        {
            g_serialWriter->Print("FAT32: ERROR - Could not read Root LBA\n");
            return;
        }

        // Check if the sector is empty
        bool allZero = true;
        for (int i = 0; i < 512; i++)
        {
            if (sectorData[i] != 0) {
                allZero = false;
                break;
            }
        }

        if (allZero)
        {
            g_serialWriter->Print("FAT32: FATAL - Sector is empty. Root directory not found.\n");
            return;
        }

        // Show first 32 bytes for debugging
        g_serialWriter->Print("FAT32: First 32 bytes: ");
        for (int i = 0; i < 32; i++)
        {
            g_serialWriter->PrintHex(sectorData[i]);
            g_serialWriter->Print(" ");
        }
        g_serialWriter->Print("\n");

        // Iterate entries
        FAT_DirectoryEntry *dir = (FAT_DirectoryEntry *)sectorData;
        g_serialWriter->Print("--- Root Directory Listing ---\n");
        
        int entry_count = 0;
        for (int i = 0; i < 512 / sizeof(FAT_DirectoryEntry); i++)
        {
            // End of directory
            if (dir[i].name[0] == 0x00)
                break;
            
            // Deleted entry
            if ((uint8_t)dir[i].name[0] == 0xE5)
                continue;
            
            // Long filename entry
            if (dir[i].attributes == FAT_ATTR_LONG_NAME)
                continue;
            
            // Volume label (skip unless you want to show it)
            if (dir[i].attributes & FAT_ATTR_VOLUME_ID)
                continue;

            entry_count++;
            g_serialWriter->Print("  ");
            
            // Print name (padded with spaces)
            for (int k = 0; k < 11; k++)
            {
                char c = dir[i].name[k];
                if (c >= 32 && c <= 126) {  // Printable ASCII
                    char str[2] = {c, 0};
                    g_serialWriter->Print(str);
                } else {
                    g_serialWriter->Print("?");
                }
            }
            
            // Print attributes
            if (dir[i].attributes & FAT_ATTR_DIRECTORY)
                g_serialWriter->Print("  [DIR]");
            else {
                g_serialWriter->Print("  [FILE] ");
                char size_buf[32];
                g_serialWriter->Print(itoa(dir[i].file_size, size_buf, 10));
                g_serialWriter->Print(" bytes");
            }
            
            g_serialWriter->Print("\n");
        }
        
        g_serialWriter->Print("Total entries: ");
        char count_buf[32];
        g_serialWriter->Print(itoa(entry_count, count_buf, 10));
        g_serialWriter->Print("\n");
    }

    void ToFatName(const char *input, char *output)
    {
        if (!input || !output) return;

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
        if (cluster < 2 || cluster >= FAT32_EOC_MIN) {
            return FAT32_EOC_MIN;  // Invalid or end of chain
        }

        uint32_t fat_offset = cluster * 4;  // FAT32 uses 4 bytes per entry
        uint32_t fat_sector = partition_start_lba + bpb.reserved_sectors + (fat_offset / 512);
        uint32_t ent_offset = fat_offset % 512;

        // Use cache if available
        if (fat_cache_valid && fat_cache_sector == fat_sector) {
            uint32_t next_cluster = *(uint32_t *)&fat_cache[ent_offset] & 0x0FFFFFFF;
            return next_cluster;
        }

        // Read FAT sector and update cache
        // Yield before I/O to allow other tasks to run
        Scheduler::Yield();
        
        if (!DiskDevice::ReadSectors(fat_sector, 1, fat_cache)) {
            g_serialWriter->Print("FAT32: Failed to read FAT sector\n");
            fat_cache_valid = false;
            return FAT32_BAD_CLUSTER;
        }

        fat_cache_sector = fat_sector;
        fat_cache_valid = true;

        // Read the entry and mask the top 4 bits (reserved in FAT32)
        uint32_t next_cluster = *(uint32_t *)&fat_cache[ent_offset] & 0x0FFFFFFF;
        return next_cluster;
    }

    void *ReadFile(const char *filepath, size_t *out_size)
    {
        if (!initialized) {
            g_serialWriter->Print("FAT32: Not initialized\n");
            return nullptr;
        }

        if (!filepath || !out_size) {
            g_serialWriter->Print("FAT32: Invalid parameters\n");
            return nullptr;
        }

        g_serialWriter->Print("FAT32: Attempting to read: ");
        g_serialWriter->Print(filepath);
        g_serialWriter->Print("\n");

        // Safety Copy: strtok is destructive
        char pathBuffer[256];
        size_t pathLen = strlen(filepath);
        if (pathLen >= 256) {
            g_serialWriter->Print("FAT32: Path too long\n");
            return nullptr;
        }
        strcpy(pathBuffer, filepath);

        uint32_t currentCluster = bpb.root_cluster;
        FAT_DirectoryEntry entry;
        memset(&entry, 0, sizeof(entry));
        bool found = false;

        // Traversal: Loop through each part of the path
        char *part = strtok(pathBuffer, "/\\");
        while (part != nullptr)
        {
            g_serialWriter->Print("FAT32: Looking for: ");
            g_serialWriter->Print(part);
            g_serialWriter->Print("\n");

            char targetFatName[11];
            ToFatName(part, targetFatName);

            g_serialWriter->Print("FAT32: FAT name: ");
            for (int i = 0; i < 11; i++) {
                char c[2] = {targetFatName[i], 0};
                g_serialWriter->Print(c);
            }
            g_serialWriter->Print("\n");

            uint32_t clusterSize = bpb.sectors_per_cluster * 512;
            uint8_t *dirBuf = (uint8_t *)g_heap.Alloc(clusterSize);
            if (!dirBuf) {
                g_serialWriter->Print("FAT32: Failed to allocate directory buffer\n");
                return nullptr;
            }

            found = false;
            uint32_t searchCluster = currentCluster;
            int iterations = 0;
            const int MAX_ITERATIONS = 1000;  // Prevent infinite loops

            // Search through the directory's cluster chain
            while (searchCluster >= 2 && searchCluster < FAT32_EOC_MIN && iterations < MAX_ITERATIONS)
            {
                iterations++;
                
                // Yield before I/O
                Scheduler::Yield();
                
                if (!DiskDevice::ReadSectors(ClusterToLBA(searchCluster), bpb.sectors_per_cluster, dirBuf)) {
                    g_serialWriter->Print("FAT32: Failed to read directory cluster\n");
                    break;
                }

                FAT_DirectoryEntry *dir = (FAT_DirectoryEntry *)dirBuf;
                for (uint32_t i = 0; i < clusterSize / sizeof(FAT_DirectoryEntry); i++)
                {
                    if (dir[i].name[0] == 0x00)
                        break; // End of directory
                    if ((uint8_t)dir[i].name[0] == 0xE5 || dir[i].attributes == FAT_ATTR_LONG_NAME)
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
                
                searchCluster = GetNextCluster(searchCluster);
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
            
            // Validate cluster number
            if (currentCluster < 2) {
                g_serialWriter->Print("FAT32: Invalid cluster number: ");
                char buf[32];
                g_serialWriter->Print(itoa(currentCluster, buf, 10));
                g_serialWriter->Print("\n");
                return nullptr;
            }
            
            part = strtok(nullptr, "/\\");
        }

        // Ensure we found a file, not a directory
        if (entry.attributes & FAT_ATTR_DIRECTORY) {
            g_serialWriter->Print("FAT32: Path refers to a directory, not a file\n");
            return nullptr;
        }

        // Data Loading: Follow the cluster chain
        *out_size = entry.file_size;
        
        if (entry.file_size == 0) {
            g_serialWriter->Print("FAT32: File is empty\n");
            return nullptr;
        }

        g_serialWriter->Print("FAT32: File size: ");
        char size_buf[32];
        g_serialWriter->Print(itoa(entry.file_size, size_buf, 10));
        g_serialWriter->Print(" bytes\n");

        uint8_t *fileData = (uint8_t *)g_heap.Alloc(entry.file_size);
        if (!fileData) {
            g_serialWriter->Print("FAT32: Failed to allocate file buffer\n");
            return nullptr;
        }

        uint32_t bytesRemaining = entry.file_size;
        uint32_t fileCluster = currentCluster;
        uint32_t clusterSize = bpb.sectors_per_cluster * 512;
        uint8_t *tempClusterBuf = (uint8_t *)g_heap.Alloc(clusterSize);
        if (!tempClusterBuf) {
            g_serialWriter->Print("FAT32: Failed to allocate temp buffer\n");
            g_heap.Free(fileData);
            return nullptr;
        }

        uint32_t offset = 0;
        int cluster_count = 0;
        const int MAX_CLUSTERS = 10000;  // Sanity check

        while (fileCluster >= 2 && fileCluster < FAT32_EOC_MIN && 
               bytesRemaining > 0 && cluster_count < MAX_CLUSTERS)
        {
            cluster_count++;
            
            char dbg_buf[32];
            g_serialWriter->Print("FAT32: Reading cluster ");
            g_serialWriter->Print(itoa(fileCluster, dbg_buf, 10));
            g_serialWriter->Print(" (");
            g_serialWriter->Print(itoa(bytesRemaining, dbg_buf, 10));
            g_serialWriter->Print(" bytes remaining)\n");

            // Yield before I/O
            Scheduler::Yield();
            
            if (!DiskDevice::ReadSectors(ClusterToLBA(fileCluster), bpb.sectors_per_cluster, tempClusterBuf))
            {
                g_serialWriter->Print("FAT32: Failed to read file cluster\n");
                g_heap.Free(tempClusterBuf);
                g_heap.Free(fileData);
                return nullptr;
            }
            
            uint32_t toCopy = (bytesRemaining > clusterSize) ? clusterSize : bytesRemaining;
            memcpy(fileData + offset, tempClusterBuf, toCopy);

            offset += toCopy;
            bytesRemaining -= toCopy;
            fileCluster = GetNextCluster(fileCluster);
        }

        g_heap.Free(tempClusterBuf);

        if (bytesRemaining > 0) {
            g_serialWriter->Print("FAT32: Warning - file not fully read (");
            char buf[32];
            g_serialWriter->Print(itoa(bytesRemaining, buf, 10));
            g_serialWriter->Print(" bytes remaining)\n");
        }

        g_serialWriter->Print("FAT32: File loaded successfully (");
        char buf[32];
        g_serialWriter->Print(itoa(cluster_count, buf, 10));
        g_serialWriter->Print(" clusters read)\n");
        
        return fileData;
    }
};