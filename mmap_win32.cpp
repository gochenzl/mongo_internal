#include "mmap.h"
#include <windows.h>
#include <string>

class MemoryMappedFileImpl
{
public:
    MemoryMappedFileImpl(const char* filename);
    ~MemoryMappedFileImpl();

    void* map();

private:
    void close();

private:
    std::string filename_;
    HANDLE fd_;
    HANDLE maphandle_;
    void* view_;
};

MemoryMappedFileImpl::MemoryMappedFileImpl(const char* filename) : filename_(filename)
{
    fd_ = INVALID_HANDLE_VALUE;
    maphandle_ = NULL;
    view_ = NULL;
}

MemoryMappedFileImpl::~MemoryMappedFileImpl()
{
    close();
}

void* MemoryMappedFileImpl::map()
{
    if (view_)
        return view_;

    fd_ = CreateFile(
        filename_.c_str(),
        GENERIC_READ, // desired access
        FILE_SHARE_READ, // share mode
        NULL, // security
        OPEN_ALWAYS, // create disposition
        FILE_ATTRIBUTE_NORMAL , // flags
        NULL); // hTempl

    if (fd_ == INVALID_HANDLE_VALUE)
        return NULL;

    DWORD flProtect = PAGE_READONLY;
    maphandle_ = CreateFileMapping(fd_, NULL, flProtect, 0, 0, NULL);
    if (maphandle_ == NULL )
    {
        CloseHandle(fd_);
        fd_ = INVALID_HANDLE_VALUE;
        return NULL;
    }

    view_ = MapViewOfFileEx(
        maphandle_,      // file mapping handle
        FILE_MAP_READ,         // access
        0, 0,           // file offset, high and low
        0,              // bytes to map, 0 == all
        NULL);  // address to place file

    return view_;
}

void MemoryMappedFileImpl::close()
{
    if (view_)
        UnmapViewOfFile(view_);

    if (maphandle_)
        CloseHandle(maphandle_);

    if (fd_ != INVALID_HANDLE_VALUE)
        CloseHandle(fd_);
}

MemoryMappedFile::MemoryMappedFile(const char* filename)
{
    pImpl_ = new MemoryMappedFileImpl(filename);
}

MemoryMappedFile::~MemoryMappedFile()
{
    delete pImpl_;
}

void* MemoryMappedFile::map()
{
    return pImpl_->map();
}

