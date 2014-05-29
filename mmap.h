#pragma once

class MemoryMappedFileImpl;
class MemoryMappedFile
{
public:
    MemoryMappedFile(const char* filename);
    ~MemoryMappedFile();

    void* map();

private:
    MemoryMappedFileImpl* pImpl_;

private:
    MemoryMappedFile(MemoryMappedFile&);
    MemoryMappedFile& operator=(MemoryMappedFile&);
};

