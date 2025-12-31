#include <efi.h>
#include <efilib.h>
#include <stdint.h>

typedef struct {
    uint64_t BaseAddress;
    uint64_t BufferSize;
    uint32_t Width;
    uint32_t Height;
    uint32_t PixelsPerScanLine;
} Framebuffer;

typedef struct {
    void* kernel_entry;
    void* rsdt_address;
    void* mmap_address;
    unsigned int mmap_size;
    unsigned int descriptor_size;
    void* font_address;
    Framebuffer framebuffer;
} BootInfo;

// Define this globally or as static so it doesn't live on the stack
static BootInfo final_boot_info;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"NeoOS Bootloader: Initializing Handover...\r\n");

    // --- 1. PROTOCOLS ---
    EFI_GUID vfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GUID acpi_guid = ACPI_20_TABLE_GUID;

    EFI_LOADED_IMAGE *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *vfs;
    EFI_FILE_PROTOCOL *root, *kernel_file;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    // --- 2. FIND ACPI TABLE ---
    void* rsdt_ptr = NULL;
    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        if (CompareGuid(&SystemTable->ConfigurationTable[i].VendorGuid, &acpi_guid) == 0) {
            rsdt_ptr = SystemTable->ConfigurationTable[i].VendorTable;
            Print(L"ACPI Table Found at: 0x%lx\r\n", rsdt_ptr);
            break;
        }
    }

    // --- 3. INITIALIZE GRAPHICS (GOP) ---
    Framebuffer* fb = NULL;
    EFI_STATUS status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if (status == EFI_SUCCESS) {
        uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, sizeof(Framebuffer), (void**)&fb);
        fb->BaseAddress = gop->Mode->FrameBufferBase;
        fb->BufferSize = gop->Mode->FrameBufferSize;
        fb->Width = gop->Mode->Info->HorizontalResolution;
        fb->Height = gop->Mode->Info->VerticalResolution;
        fb->PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;
    }

    // --- 4. LOAD KERNEL FILE ---
    uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, ImageHandle, &loaded_image_guid, (void**)&loaded_image);
    uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, loaded_image->DeviceHandle, &vfs_guid, (void**)&vfs);
    uefi_call_wrapper(vfs->OpenVolume, 2, vfs, &root);
    
    status = uefi_call_wrapper(root->Open, 5, root, &kernel_file, L"\\NEOOS\\KERNEL.BIN", EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) {
        Print(L"Kernel not found!\r\n");
        return status;
    }

    EFI_FILE_INFO *info;
    UINTN info_size = sizeof(EFI_FILE_INFO) + 1024;
    uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, info_size, (void**)&info);
    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    uefi_call_wrapper(kernel_file->GetInfo, 4, kernel_file, &file_info_guid, &info_size, info);
    UINTN kernel_size = info->FileSize;

    EFI_PHYSICAL_ADDRESS kernel_buffer = 0x2000000; 
    UINTN pages = (kernel_size + 4095) / 4096; 
    status = uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAddress, EfiLoaderCode, pages, &kernel_buffer);

    UINTN read_size = kernel_size;
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &read_size, (void*)kernel_buffer);
    

    
    EFI_FILE_PROTOCOL *font_file;
    status = uefi_call_wrapper(root->Open, 5, root, &font_file, L"\\NEOOS\\FONT.PSF", EFI_FILE_MODE_READ, 0);

    if (status == EFI_SUCCESS) {
        // 1. Get file size
        UINTN f_info_size = sizeof(EFI_FILE_INFO) + 1024;
        EFI_FILE_INFO *f_info;
        uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, f_info_size, (void**)&f_info);
        uefi_call_wrapper(font_file->GetInfo, 4, font_file, &file_info_guid, &f_info_size, f_info);
        
        // 2. Allocate memory for font
        UINTN font_size = f_info->FileSize;
        EFI_PHYSICAL_ADDRESS font_buffer;
        uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, (font_size + 4095) / 4096, &font_buffer);
        
        // 3. Read file into memory
        uefi_call_wrapper(font_file->Read, 3, font_file, &font_size, (void*)font_buffer);
        
        // 4. Pass to final_boot_info
        final_boot_info.font_address = (void*)font_buffer;
    }

    // --- 5. EXIT BOOT SERVICES RETRY LOOP ---
    UINTN mmap_size = 0, map_key, descriptor_size;
    UINT32 descriptor_version;
    EFI_MEMORY_DESCRIPTOR* mmap = NULL;

    // First call to get the buffer size needed
    uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &mmap_size, NULL, &map_key, &descriptor_size, &descriptor_version);
    
    // Allocate slightly more just in case
    mmap_size += (4 * descriptor_size); 
    uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, mmap_size, (void**)&mmap);

    // Final attempt to get the map and immediately exit
    status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &mmap_size, mmap, &map_key, &descriptor_size, &descriptor_version);
    if (status == EFI_SUCCESS) {
        status = uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2, ImageHandle, map_key);
    }

    if (EFI_ERROR(status)) {
        Print(L"CRITICAL ERROR: Could not exit boot services! Status: %d\r\n", status);
        // Wait so you can read the error on screen
        uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 5000000); 
        return status;
    }



    // --- 6. PREPARE FINAL HANDOVER DATA (MUST BE AFTER LOOP) ---
    final_boot_info.kernel_entry    = (void*)(uintptr_t)kernel_buffer;
    final_boot_info.rsdt_address    = rsdt_ptr;
    final_boot_info.framebuffer     = *fb;
    final_boot_info.mmap_address    = (void*)mmap;
    final_boot_info.mmap_size       = (unsigned int)mmap_size;
    final_boot_info.descriptor_size = (unsigned int)descriptor_size;

    // --- 7. THE JUMP ---
    uint64_t jump_target = (uint64_t)kernel_buffer;

    __asm__ volatile (
        "cli\n\t"
        "mov %0, %%rdi\n\t"      // Pass &final_boot_info in RDI
        "mov %1, %%rax\n\t"
        "jmp *%%rax"
        :
        : "r"(&final_boot_info), "r"(jump_target)
        : "rax", "rdi"
    );

    return EFI_SUCCESS;
}