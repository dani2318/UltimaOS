#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <libs/core/UEFI.h>


typedef void (*KernelStart)(BootInfo*, EFI_SYSTEM_TABLE *, EFI_HANDLE*);

static BootInfo final_boot_info;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"NeoOS Bootloader: Initializing Handover...\r\n");

    EFI_GUID vfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GUID acpi_guid = ACPI_20_TABLE_GUID;

    EFI_LOADED_IMAGE *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *vfs;
    EFI_FILE_PROTOCOL *root, *kernel_file;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    void* rsdt_ptr = NULL;
    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        if (CompareGuid(&SystemTable->ConfigurationTable[i].VendorGuid, &acpi_guid) == 0) {
            rsdt_ptr = SystemTable->ConfigurationTable[i].VendorTable;
            Print(L"ACPI Table Found at: 0x%lx\r\n", rsdt_ptr);
            break;
        }
    }

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

    if (EFI_ERROR(status)) {
        Print(L"ERROR: Could not allocate memory for kernel at 0x2000000!\r\n");
        return status;
    }

    UINTN read_size = kernel_size;
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &read_size, (void*)kernel_buffer);
    
    Print(L"Kernel loaded at 0x%lx, size: %lu bytes\r\n", kernel_buffer, kernel_size);

    EFI_FILE_PROTOCOL *font_file;
    status = uefi_call_wrapper(root->Open, 5, root, &font_file, L"\\NEOOS\\FONT.PSF", EFI_FILE_MODE_READ, 0);

    if (status == EFI_SUCCESS) {
        UINTN f_info_size = sizeof(EFI_FILE_INFO) + 1024;
        EFI_FILE_INFO *f_info;
        uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, f_info_size, (void**)&f_info);
        uefi_call_wrapper(font_file->GetInfo, 4, font_file, &file_info_guid, &f_info_size, f_info);
        
        UINTN font_size = f_info->FileSize;
        EFI_PHYSICAL_ADDRESS font_buffer;
        uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, (font_size + 4095) / 4096, &font_buffer);
        
        uefi_call_wrapper(font_file->Read, 3, font_file, &font_size, (void*)font_buffer);
        
        final_boot_info.font_address = (void*)font_buffer;
        Print(L"Font loaded successfully\r\n");
    } else {
        Print(L"Warning: Font file not found\r\n");
        final_boot_info.font_address = NULL;
    }

    UINTN mmap_size = 0, map_key, descriptor_size;
    UINT32 descriptor_version;
    EFI_MEMORY_DESCRIPTOR* mmap = NULL;

    uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &mmap_size, NULL, &map_key, &descriptor_size, &descriptor_version);
    
    mmap_size += (4 * descriptor_size); 
    uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, mmap_size, (void**)&mmap);

    status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &mmap_size, mmap, &map_key, &descriptor_size, &descriptor_version);
    
    if (EFI_ERROR(status)) {
        Print(L"ERROR: Could not get memory map! Status: %d\r\n", status);
        uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 5000000); 
        return status;
    }

    Print(L"Memory map retrieved successfully\r\n");
    Print(L"Boot Services remain active - kernel will handle them\r\n");

  
    final_boot_info.kernel_entry    = (void*)(uintptr_t)kernel_buffer;
    final_boot_info.rsdt_address    = rsdt_ptr;
    final_boot_info.framebuffer     = *fb;
    final_boot_info.mmap_address    = (void*)mmap;
    final_boot_info.mmap_size       = (unsigned int)mmap_size;
    final_boot_info.descriptor_size = (unsigned int)descriptor_size;

    Print(L"Jumping to kernel at 0x%lx...\r\n", kernel_buffer);
    
    uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 500000);

    KernelStart kernel_start = (KernelStart)kernel_buffer;
    
    __asm__ volatile ("cli");
    
    kernel_start(&final_boot_info, SystemTable, &ImageHandle);

    Print(L"ERROR: Kernel returned control to bootloader!\r\n");
    while(1) {
        __asm__ volatile ("hlt");
    }
    
    return EFI_SUCCESS;
}