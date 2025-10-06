#pragma once
#include <core/dev/BlockDevice.hpp>
#include "File.hpp"
#include "FileEntry.hpp"
#include <stdbool.h>
#define MAX_PATH_SIZE 256

class FileSystem{
    public:
        virtual bool Initialize(BlockDevice* device) = 0;
        virtual File* RootDirectory() = 0;
        File* Open(const char* path, FileOpenMode mode);
    private:
        virtual FileEntry* FindFile(File* parent, const char* name);
};