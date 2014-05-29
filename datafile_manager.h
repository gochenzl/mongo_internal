#pragma once
#include <vector>
#include <string>
#include "pdfile.h"

class MemoryMappedFile;

class DatafileManager
{
public:
    DatafileManager(const std::string& dbpath, const std::string& dbname);
    void* getView(int n, int offset = 0);
    
private:
    std::string dbpath_;
    std::string dbname_;
    std::vector<MemoryMappedFile*> mmfiles_;

private:
    DatafileManager(DatafileManager&);
    DatafileManager& operator=(DatafileManager&);
};

