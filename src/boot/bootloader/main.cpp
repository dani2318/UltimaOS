#include <stdint.h>
#include <memory/memdefs.h>
#include <arch/i686/RealMemory.hpp>
#include <dev/MBR.hpp>
#include <arch/i686/BiosDisk.hpp>
#include <memory/Stage2Allocator.hpp>
#include <cpp/NewDelete.hpp>

#include <core/Defs.hpp>
#include <core/arch/i686/VGATextDevice.hpp>
#include <core/arch/i686/E9Device.hpp>
#include <core/dev/TextDevice.hpp>
#include <core/dev/RangedBlockDevice.hpp>
#include <core/Debug.hpp>
#include <boot/bootparams.hpp>
#include <core/FS/FATFileSystem.hpp>


arch::i686::VGATextDevice VGADevice;
arch::i686::E9Device E9Device;
Stage2Allocator g_Allocator(reinterpret_cast<void*>(MEMORY_MIN), MEMORY_MAX - MEMORY_MIN);
EXPORT void ASMCALL Start(uint16_t bootDrive, uint32_t partition){
    SetCppAlloc(&g_Allocator);

    VGADevice.Clear();
    
    TextDevice screen(&VGADevice);
    Debug::AddOutDevice(Debug::Level::INFO, false, &screen);
    TextDevice debug(&E9Device);
    Debug::AddOutDevice( Debug::Level::DEBUG, true, &debug);

    Debug::Info("stage2","bootDrive: %x partition: %x ",bootDrive, partition);

    BIOSDisk disk(bootDrive);
    if (!disk.Initialize())
    {
        Debug::Critical("stage2", "Failed to initialize disk!");
        for (;;);
    }
    Debug::Info("stage2", "Disk OK!");

    // Handle partitioned disks
    BlockDevice* part;
    RangeBlockDevice partRange;
    uint32_t partitionLBA;
    if (bootDrive < 0x80) {
        part = &disk;
    } else {
        MBREntry* entry = to_linear<MBREntry*>(partition); // if you really need the MBR structure
        partitionLBA = entry->LbaStart;   // partition = LBA start
        Debug::Info("stage2", "to_linear<uint32_t>(partition): 0x%x", to_linear<uint32_t>(partition));
        Debug::Info("stage2", "================ MBREntry info ================");
        Debug::Info("stage2", "====== Attributes: %d", entry->Attributes);
        Debug::Info("stage2", "== ChsStart ==");
        for(int i = 0; i < 3; i++){
            Debug::Info("stage2", "%x", entry->ChsStart[i]);
        }
        Debug::Info("stage2", "== ChsStart ==");

        Debug::Info("stage2", "====== PartitionType: %d", entry->PartitionType);
        Debug::Info("stage2", "== ChsEnd ==");
        for(int i = 0; i < 3; i++){
            Debug::Info("stage2", "%x", entry->ChsEnd[i]);
        }
        Debug::Info("stage2", "== ChsEnd ==");
        Debug::Info("stage2", "====== LbaStart: %d", entry->LbaStart);
        Debug::Info("stage2", "====== Size: %d", entry->Size);
        
        Debug::Info("stage2", "================ MBREntry info ================");

        
        Debug::Info("stage2", "MBR partition start: %u, size: %u", entry->LbaStart, entry->Size);
        partRange.Initialize(&disk, partitionLBA, entry->Size);
        Debug::Info("stage2", "partRange size: %d partRange position: %d", partRange.Size(), partRange.Position());
        part = &partRange;
    }

    Debug::Info("stage2", "Partition OK!");

    // Read partition
    FATFileSystem fs;
    if (!fs.Initialize(part))
    {
        Debug::Critical("stage2", "Failed to initialize FAT file system!");
        for (;;);
    }

    Debug::Info("stage2", "FAT filesystem load success!");
    File* kernel = fs.Open("NeoOS/System/kernel.elf", FileOpenMode::Read);

    Debug::Info("stage2", "OK!");

    for (;;);
}