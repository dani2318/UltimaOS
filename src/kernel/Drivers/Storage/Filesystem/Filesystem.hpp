#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Drivers/HAL/HAL.hpp>
#include <memory/memory.hpp>
#include <arch/x86_64/Serial.hpp>
#include <globals.hpp>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <arch/x86_64/ScreenWriter.hpp>

class FileSystem
{
public:
    static void AsciiToUnicode(const char *src, CHAR16 *dst, size_t max_len = 256)
    {
        size_t i = 0;
        while (*src && i < max_len - 1)
        {
            *dst++ = (CHAR16)(*src++);
            i++;
        }
        *dst = 0; // null-terminate
    }

    // Reads the entire file from ESP and returns a heap-allocated buffer
    static void *ReadAll(const char *path8, size_t *out_size)
    {
        if (!path8 || !out_size) {
            g_serialWriter->Print("FS: Invalid parameters\n");
            return nullptr;
        }

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = nullptr;
        EFI_FILE_PROTOCOL *root = nullptr;
        EFI_FILE_PROTOCOL *file = nullptr;

        CHAR16 path[256];
        AsciiToUnicode(path8, path, 256);

        // Get filesystem protocol
        EFI_STATUS status = gST->BootServices->HandleProtocol(
            gImageHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&fs);
        if (EFI_ERROR(status))
        {
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
            gScreenwriter->print("FS: Failed to open FS protocol\n", true);
=======
            g_serialWriter->Print("FS: Failed to open FS protocol (status=0x");
            g_serialWriter->PrintHex(status);
            g_serialWriter->Print(")\n");
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            return nullptr;
        }

        // Open root volume
        status = fs->OpenVolume(fs, &root);
        if (EFI_ERROR(status))
        {
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
            gScreenwriter->print("FS: Failed to open root volume\n", true);
=======
            g_serialWriter->Print("FS: Failed to open root volume (status=0x");
            g_serialWriter->PrintHex(status);
            g_serialWriter->Print(")\n");
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            return nullptr;
        }

        // Try to open the file
        status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(status))
        {
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
            gScreenwriter->print("FS: File not found: ", true);
            gScreenwriter->print(path8, true);
            gScreenwriter->print("\n", true);
            return nullptr; // << immediately return
=======
            g_serialWriter->Print("FS: File not found: ");
            g_serialWriter->Print(path8);
            g_serialWriter->Print(" (status=0x");
            g_serialWriter->PrintHex(status);
            g_serialWriter->Print(")\n");
            root->Close(root);
            return nullptr;
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
        }

        // Get file size
        UINTN buf_size = sizeof(EFI_FILE_INFO) + 256;
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
        EFI_FILE_INFO *info = (EFI_FILE_INFO *)gHeap.alloc(buf_size);
        if (!info)
=======
        EFI_FILE_INFO *info = (EFI_FILE_INFO *)g_heap.Alloc(buf_size);
        if (!info) {
            g_serialWriter->Print("FS: Failed to allocate info buffer\n");
            file->Close(file);
            root->Close(root);
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            return nullptr;
        }

        status = file->GetInfo(file, &gEfiFileInfoGuid, &buf_size, info);
        if (EFI_ERROR(status))
        {
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
            gScreenwriter->print("FS: Failed to get file info\n", true);
=======
            g_serialWriter->Print("FS: Failed to get file info (status=0x");
            g_serialWriter->PrintHex(status);
            g_serialWriter->Print(")\n");
            g_heap.Free(info);
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            file->Close(file);
            root->Close(root);
            return nullptr;
        }

        UINTN file_size = info->FileSize;
        g_heap.Free(info);

        // Sanity check file size
        if (file_size == 0) {
            g_serialWriter->Print("FS: File is empty\n");
            file->Close(file);
            root->Close(root);
            return nullptr;
        }

        if (file_size > 100 * 1024 * 1024) {  // 100MB sanity check
            g_serialWriter->Print("FS: File too large (");
            char size_buf[32];
            g_serialWriter->Print(itoa(file_size, size_buf, 10));
            g_serialWriter->Print(" bytes)\n");
            file->Close(file);
            root->Close(root);
            return nullptr;
        }

        // Allocate buffer
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
        UINTN read_size = info->FileSize;
        void *buffer = gHeap.alloc(read_size);
        if (!buffer)
        {
            gScreenwriter->print("FS: Heap allocation failed\n", true);
=======
        void *buffer = g_heap.Alloc(file_size);
        if (!buffer)
        {
            g_serialWriter->Print("FS: Heap allocation failed for ");
            char size_buf[32];
            g_serialWriter->Print(itoa(file_size, size_buf, 10));
            g_serialWriter->Print(" bytes\n");
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            file->Close(file);
            root->Close(root);
            return nullptr;
        }

        // Read file
        UINTN read_size = file_size;
        status = file->Read(file, &read_size, buffer);
        if (EFI_ERROR(status))
        {
<<<<<<< Updated upstream:src/kernel/Drivers/Storage/Filesystem/Filesystem.hpp
            gScreenwriter->print("FS: File read failed\n", true);
            gHeap.free(buffer);
=======
            g_serialWriter->Print("FS: File read failed (status=0x");
            g_serialWriter->PrintHex(status);
            g_serialWriter->Print(")\n");
            g_heap.Free(buffer);
>>>>>>> Stashed changes:src/kernel/arch/x86_64/Filesystem/Filesystem.hpp
            file->Close(file);
            root->Close(root);
            return nullptr;
        }

        // Verify read size matches expected
        if (read_size != file_size) {
            g_serialWriter->Print("FS: Warning - read size mismatch (expected=");
            char buf[32];
            g_serialWriter->Print(itoa(file_size, buf, 10));
            g_serialWriter->Print(", got=");
            g_serialWriter->Print(itoa(read_size, buf, 10));
            g_serialWriter->Print(")\n");
        }

        *out_size = read_size;
        file->Close(file);
        root->Close(root);
        
        g_serialWriter->Print("FS: Successfully read ");
        char size_buf[32];
        g_serialWriter->Print(itoa(read_size, size_buf, 10));
        g_serialWriter->Print(" bytes from ");
        g_serialWriter->Print(path8);
        g_serialWriter->Print("\n");
        
        return buffer;
    }
};