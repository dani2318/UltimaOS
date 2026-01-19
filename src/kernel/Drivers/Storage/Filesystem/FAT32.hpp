#pragma once
#include <stdint.h>
#include <Drivers/Storage/Filesystem/GPT.hpp>
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
class FAT32Filesystem {
private:
    uint64_t partition_start_lba;
    FAT32_BPB bpb;

    // Helper to read the actual file data once the cluster is found
    void* InternalReadClusterChain(uint32_t firstCluster, uint32_t fileSize) {
        if (fileSize == 0) return nullptr;

        void* fileData = gHeap.alloc(fileSize);
        if (!fileData) return nullptr;

        uint32_t clusterSize = bpb.sectors_per_cluster * 512;
        uint8_t* tempBuf = (uint8_t*)gHeap.alloc(clusterSize);

        uint32_t currentCluster = firstCluster;
        uint32_t offset = 0;
        uint32_t bytesRemaining = fileSize;

        while (currentCluster < 0x0FFFFFF8 && bytesRemaining > 0) {
            uint64_t lba = ClusterToLBA(currentCluster);
            DiskDevice::ReadSectors(lba, bpb.sectors_per_cluster, tempBuf);

            uint32_t toCopy = (bytesRemaining > clusterSize) ? clusterSize : bytesRemaining;
            memcpy((uint8_t*)fileData + offset, tempBuf, toCopy);

            offset += toCopy;
            bytesRemaining -= toCopy;
            currentCluster = GetNextCluster(currentCluster);
        }

        gHeap.free(tempBuf);
        return fileData;
    }

public:
    FAT32Filesystem(uint64_t start_lba) : partition_start_lba(start_lba) {
        // Use a local buffer to read the BPB
        uint8_t sector0[512];
        DiskDevice::ReadSectors(partition_start_lba, 1, sector0);

        // Copy the data into our class member
        memcpy(&bpb, sector0, sizeof(FAT32_BPB));

        gScreenwriter->print("FAT32: Initialized. Root Cluster: ", true);
        char b[32];
        gScreenwriter->print(itoa(bpb.root_cluster, b, 10), true);
        gScreenwriter->print("\n", true);
    }

    uint64_t ClusterToLBA(uint32_t cluster) {
        uint64_t data_sec = bpb.reserved_sectors + (bpb.fat_count * bpb.sectors_per_fat_32);
        return partition_start_lba + data_sec + ((cluster - 2) * bpb.sectors_per_cluster);
    }

    uint32_t GetNextCluster(uint32_t cluster) {
        uint32_t fat_sector = bpb.reserved_sectors + (cluster * 4 / 512);
        uint32_t fat_offset = (cluster * 4) % 512;
        uint8_t buf[512];

        if (!DiskDevice::ReadSectors(partition_start_lba + fat_sector, 1, buf)) return 0x0FFFFFF7;
        return *(uint32_t*)&buf[fat_offset] & 0x0FFFFFFF;
    }

    void* ReadFile(const char* path, uint32_t* fileSizeOut) {
        char pathCopy[256];
        uint16_t pathLen = strlen(path);
        memcpy(pathCopy, path, pathLen + 1);

        uint32_t currentDirCluster = bpb.root_cluster;
        char* currentPart = pathCopy;
        char* nextPart = nullptr;

        while (currentPart != nullptr) {
            nextPart = strchr(currentPart, '/');
            if (nextPart != nullptr) {
                *nextPart = '\0';
                nextPart++;
            }

            // Convert current name to FAT format
            char fatName[11];
            memset(fatName, ' ', 11);
            int j = 0;
            for (int i = 0; i < 8 && currentPart[i] != '\0' && currentPart[i] != '.'; i++) {
                char c = currentPart[i];
                if (c >= 'a' && c <= 'z') c -= 32;
                fatName[i] = c;
            }
            char* dot = strchr(currentPart, '.');
            if (dot) {
                dot++;
                for (int i = 0; i < 3 && dot[i] != '\0'; i++) {
                    char c = dot[i];
                    if (c >= 'a' && c <= 'z') c -= 32;
                    fatName[8 + i] = c;
                }
            }

            // Search directory
            uint8_t dirBuf[512]; // Sector buffer
            bool found = false;
            uint32_t searchCluster = currentDirCluster;

            while (searchCluster < 0x0FFFFFF8) {
                DiskDevice::ReadSectors(ClusterToLBA(searchCluster), 1, dirBuf);
                FAT_DirectoryEntry* entry = (FAT_DirectoryEntry*)dirBuf;

                for (int i = 0; i < 512 / sizeof(FAT_DirectoryEntry); i++) {
                    if (entry[i].name[0] == 0x00) break;
                    if (memcmp(entry[i].name, fatName, 11) == 0) {
                        uint32_t cluster = ((uint32_t)entry[i].first_cluster_high << 16) | entry[i].first_cluster_low;

                        if (nextPart == nullptr) {
                            // File found
                            *fileSizeOut = entry[i].file_size;
                            return InternalReadClusterChain(cluster, entry[i].file_size);
                        } else {
                            // Subdirectory found
                            currentDirCluster = cluster;
                            found = true;
                            break;
                        }
                    }
                }
                if (found) break;
                searchCluster = GetNextCluster(searchCluster);
            }

            if (!found) return nullptr;
            currentPart = nextPart;
        }
        return nullptr;
    }
};
