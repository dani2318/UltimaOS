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

BootParameters bootParams;
Stage2Allocator g_Allocator(reinterpret_cast<void*>(MEMORY_MIN), MEMORY_MAX - MEMORY_MIN);
EXPORT void ASMCALL Start(uint16_t bootDrive,uint32_t partition){
    
    SetCppAlloc(&g_Allocator);
    VGADevice.Clear();

    TextDevice Screen(&VGADevice);
    Debug::AddOutDevice(Debug::Level::INFO, false, &Screen);
    TextDevice DebugScreen(&E9Device);
    Debug::AddOutDevice(Debug::Level::DEBUG, true, &DebugScreen);
    bootParams.BootDevice = bootDrive;

    BIOSDisk disk(bootDrive);
    if(!disk.Initialize()){
        Debug::Critical("Stage2", "[CRITICAL] Failed to initialize disk!");
    }
    Debug::Info("Stage2", "[OK] Initialize disk!");

    BlockDevice* part;
    RangedBlockDevice partitionRange;
    if(bootDrive < 0x80){
        part = &disk;
    }else{
        MBREntry* entry = to_Linear<MBREntry*>(partition);
        partitionRange.Initialize(&disk, entry->LbaStart, entry->Size);
        part = &partitionRange;
    }

    Debug::Info("Stage2", "[OK] Checking disk partition!");
    FATFileSystem fs;
    if(!fs.Initialize(part)){
        Debug::Critical("Stage2", "[CRITICAL] Failed to initialize FATFileSystem!");
    }

    File* kernel = fs.Open("kernel.bin", FileOpenMode::Read);

end:
    for(;;);

}