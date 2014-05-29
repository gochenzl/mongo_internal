#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "mmap.h"

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
    int fd_;
    unsigned long long filelen_;
    void* view_;
};

MemoryMappedFileImpl::MemoryMappedFileImpl(const char* filename) : filename_(filename), fd_(-1), filelen_(0), view_(NULL)
{
    
}

MemoryMappedFileImpl::~MemoryMappedFileImpl()
{
    close();
}

void* MemoryMappedFileImpl::map()
{
    if (view_)
        return view_;
        
    fd_ = ::open(filename_.c_str(), O_RDONLY);
    if (fd_ < 0)
        return NULL;

    unsigned long long filelen_ = ::lseek(fd_, 0, SEEK_END);
    ::lseek(fd_, 0, SEEK_SET);

    view_ = ::mmap(NULL, filelen_, PROT_READ, MAP_SHARED, fd_, 0);

    return view_;
}

void MemoryMappedFileImpl::close()
{
    if (view_)
        ::munmap(view_, filelen_);

    if (fd_ >= 0)
        ::close(fd_);
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

