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

    Debug::Info("stage2","bootDrive: %u partition: %u ",bootDrive, partition);

    BIOSDisk disk(bootDrive);
    if (!disk.Initialize())
    {
        Debug::Critical("stage2", "Failed to initialize disk!");
        for (;;);
    }

    // Handle partitioned disks
    BlockDevice* part;
    RangeBlockDevice partRange;

    if (bootDrive < 0x80) {
        part = &disk;
    } else {
        MBREntry* entry = to_linear<MBREntry*>(partition); // if you really need the MBR structure
        uint32_t partitionLBA = entry->LbaStart;   // partition = LBA start
       
        Debug::Info("stage2", "MBR partition start: %u, size: %u", entry->LbaStart, entry->Size);
        partRange.Initialize(&disk, partitionLBA, entry->Size);
        Debug::Info("stage2", "partRange size: %d partRange position: %d", partRange.Size(), partRange.Position());
        part = &partRange;
    }


    // Read partition
    FATFileSystem fs;
    if (!fs.Initialize(part))
    {
        Debug::Critical("stage2", "Failed to initialize FAT file system!");
        for (;;);
    }

    Debug::Info("stage2", "FAT filesystem load success!");
    File* kernel = fs.Open("kernel.bin", FileOpenMode::Read);

    Debug::Info("stage2", "OK!");

    for (;;);
}