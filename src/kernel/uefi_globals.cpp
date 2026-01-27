// uefi_globals.cpp
#include "globals.hpp"

// Globals
EFI_HANDLE gImageHandle = nullptr;
EFI_SYSTEM_TABLE* gST = nullptr;

// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID
EFI_GUID gEfiSimpleFileSystemProtocolGuid = {
    0x0964e5b22UL, 0x6459, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

// EFI_FILE_INFO_GUID (or EFI_FILE_INFO_ID)
EFI_GUID gEfiFileInfoGuid = {
    0x09576e92UL, 0x6d3f, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};
