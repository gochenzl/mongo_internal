#include <stdio.h>
#include "datafile_manager.h"
#include "mmap.h"

static const int MaxFiles = DiskLoc::MaxFiles;

DatafileManager::DatafileManager(const std::string& dbpath, const std::string& dbname)
    : dbpath_(dbpath),
      dbname_(dbname),
      mmfiles_(MaxFiles)
{

}

void* DatafileManager::getView(int n, int offset)
{
    if (n >= MaxFiles)
        return NULL;

    if (mmfiles_[n])
        return (char*)mmfiles_[n]->map() + offset;

    char buf[100];
    sprintf(buf, "%d", n);
    std::string filename = dbpath_ + "/" + dbname_ + "." + buf;
    mmfiles_[n] = new MemoryMappedFile(filename.c_str());
    return (char*)mmfiles_[n]->map() + offset;
}



