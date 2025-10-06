#include "FileSystem.hpp"
#include <core/Debug.hpp>
#include <core/cpp/String.hpp>
#include <core/cpp/Memory.hpp>

#define module_name "FileSystem"


File* FileSystem::Open(const char* path, FileOpenMode mode){
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;


    File* root = this->RootDirectory();

    while (*path) {
        // extract next file name from path
        bool isLast = false;
        const char* delim = String::Find(path, '/');
        if (delim != NULL)
        {
            Memory::Copy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = String::Length(path);
            Memory::Copy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }
        FileEntry* nextEntry = FindFile(root, name);
        if (nextEntry)
        {
            //Release current
            root->Release();

            // check if directory
            if (!isLast && nextEntry->Type() == FileType::Directory)
            {
                Debug::Error(module_name,"%s not a directory\r\n", name);
                return nullptr;
            }

            // open new directory entry
            root = nextEntry->Open(isLast? mode : FileOpenMode::Read);
            nextEntry->Release();
        }
        else
        {
            //Release root
            root->Release();

            Debug::Error(module_name,"%s not found\r\n", name);
            return nullptr;
        }
    }

    return root;
}


FileEntry* FileSystem::FindFile(File* parent, const char* name){
    Debug::Info(module_name,"Searching for: %s", name);

    // find directory entry in current directory
    FileEntry* entry = parent->ReadFileEntry();
    while(entry != nullptr){
        if(String::Compare(entry->Name(), name) == 0){
            return entry;
        }else{
            entry->Release();
            entry = parent->ReadFileEntry();
        }

    }
    return nullptr;
}
