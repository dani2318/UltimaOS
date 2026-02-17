#pragma once

#include <core/dev/BlockDevice.hpp>

class FileEntry;

class File : BlockDevice{
    public:
        virtual FileEntry* ReadFileEntry() = 0;
        virtual void Release() = 0;
};
