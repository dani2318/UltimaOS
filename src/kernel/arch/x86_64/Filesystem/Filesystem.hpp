#pragma once
#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <memory/memory.hpp>
#include <arch/x86_64/Serial.hpp>
#include <globals.hpp>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

class FileSystem
{
public:
    static void AsciiToUnicode(const char *src, CHAR16 *dst)
    {
        while (*src)
        {
            *dst++ = (CHAR16)(*src++);
        }
        *dst = 0; // null-terminate
    }

    // Reads the entire file from ESP and returns a heap-allocated buffer
    static void *ReadAll(const char *path8, size_t *out_size)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = nullptr;
        EFI_FILE_PROTOCOL *root = nullptr;
        EFI_FILE_PROTOCOL *file = nullptr;

        CHAR16 path[256];
        AsciiToUnicode(path8, path);

        EFI_STATUS status = gST->BootServices->HandleProtocol(
            gImageHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&fs);
        if (EFI_ERROR(status))
        {
            g_serialWriter->Print("FS: Failed to open FS protocol\n");
            return nullptr;
        }

        status = fs->OpenVolume(fs, &root);
        if (EFI_ERROR(status))
        {
            g_serialWriter->Print("FS: Failed to open root volume\n");
            return nullptr;
        }

        // Try to open the file
        status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(status))
        {
            g_serialWriter->Print("FS: File not found: ");
            g_serialWriter->Print(path8);
            g_serialWriter->Print("\n");
            return nullptr; // << immediately return
        }

        // Get file size
        UINTN buf_size = sizeof(EFI_FILE_INFO) + 256;
        EFI_FILE_INFO *info = (EFI_FILE_INFO *)g_heap.Alloc(buf_size);
        if (!info)
            return nullptr;

        status = file->GetInfo(file, &gEfiFileInfoGuid, &buf_size, info);
        if (EFI_ERROR(status))
        {
            g_serialWriter->Print("FS: Failed to get file info\n");
            file->Close(file);
            return nullptr;
        }

        // Allocate buffer
        UINTN read_size = info->FileSize;
        void *buffer = g_heap.Alloc(read_size);
        if (!buffer)
        {
            g_serialWriter->Print("FS: Heap allocation failed\n");
            file->Close(file);
            return nullptr;
        }

        // Read file
        status = file->Read(file, &read_size, buffer);
        if (EFI_ERROR(status))
        {
            g_serialWriter->Print("FS: File read failed\n");
            g_heap.Free(buffer);
            file->Close(file);
            return nullptr;
        }

        *out_size = read_size;
        file->Close(file);
        return buffer;
    }
};
