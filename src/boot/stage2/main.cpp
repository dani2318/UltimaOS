#include <stdint.h>
#include <Memory/memdefs.h>
#include <arch/i686/RealMemory.hpp>
#include <dev/MBR.hpp>
#include <arch/i686/BiosDisk.hpp>
#include <Memory/Stage2Allocator.hpp>
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
    TextDevice Screen(&VGADevice);
    Debug::AddOutDevice(Debug::Level::INFO, false, &Screen);
    TextDevice DebugScreen(&E9Device);
    Debug::AddOutDevice(Debug::Level::DEBUG, true, &DebugScreen);
    
    Debug::Info("Stage2", "Starting...");
    Debug::Info("Stage2", "Boot drive received");
    Debug::Info("Stage2", "Partition received");
    
    Debug::Info("Stage2", "Creating BIOSDisk object...");
    BIOSDisk disk(bootDrive);
    
    Debug::Info("Stage2", "BIOSDisk created, calling Initialize...");
    if(!disk.Initialize()){
        Debug::Critical("Stage2", "Failed to initialize disk!");
        for(;;);
    }
    Debug::Info("Stage2", "[OK] Initialize disk!");
    
    BlockDevice* part;
    RangeBlockDevice partitionRange;
    
    Debug::Info("Stage2", "[OK] Checking disk partition!");
    
    if(bootDrive < 0x80){
        Debug::Info("Stage2", "Floppy mode - no partition");
        part = &disk;
    }else{
        Debug::Info("Stage2", "HD mode - reading partition");
        MBREntry* entry = to_linear<MBREntry*>(partition);
        partitionRange.Initialize(&disk, entry->LbaStart, entry->Size);
        part = &partitionRange;
    }
    
    FATFileSystem fs;
    if(!fs.Initialize(part)){
        Debug::Critical("Stage2", "Failed to initialize FATFileSystem!");
        for(;;);
    }
    
    Debug::Info("Stage2", "Success!");
    
    File* kernel = fs.Open("kernel.elf", FileOpenMode::Read);
end:
    for(;;);
}