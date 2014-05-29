#include <stdio.h>
#include <string.h>
#include <string>
#include "pdfile.h"
#include "mmap.h"
#include "util.h"
#include "datafile_manager.h"

static void writeData(FILE* fp, const char* buf, size_t size)
{
    size_t toWrite = size;
    size_t written = 0;

    while (toWrite)
    {
        size_t ret = fwrite(buf + written, 1, toWrite, fp);
        toWrite -= ret;
        written += ret;
    }
}

// when collection is dropped, all its extents will add to a free extent double linked list
// extent has a field nsDiagnostic which save the name of namespace, we can use it to distinguish
// if some extents were reused, these are lost
// if collection has been renamed, it can't recovered
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        printf("Usage: ./recover_collection dbpath dbname collname\n");
        return -1;
    }

    std::string dbpath(argv[1]);
    std::string dbname(argv[2]);
    std::string collname = dbname + "." + argv[3];

    DatafileManager datafileManager(dbpath, dbname);
    DataFileHeader* dataFileHeader = (DataFileHeader*)datafileManager.getView(0, 0);
    if (dataFileHeader == NULL)
    {
        printf("datafile error!\n");
        return -1;
    }

    std::string bsonFilename = std::string(argv[3]) + ".bson";
    FILE* fp = fopen(bsonFilename.c_str(), "wb");
    if (fp == NULL)
    {
        printf("open file %s fail\n", bsonFilename.c_str());
        return -1;
    }

    DiskLoc freeExtentLoc = dataFileHeader->freeListStart;
    while (!freeExtentLoc.isNull())
    {
        Extent* extent = (Extent*)datafileManager.getView(freeExtentLoc._a, freeExtentLoc.ofs);
        if (extent == NULL)
        {
            printf("datafile error!\n");
            return -1;
        }

        if (strcmp(extent->nsDiagnostic.buf, collname.c_str()) != 0)
        {
            printf("find extent belong to namespace %s, ignore it\n", extent->nsDiagnostic.buf);
            freeExtentLoc = extent->xnext;
            continue;
        }

        freeExtentLoc = extent->xnext;
        DiskLoc recordLoc = extent->firstRecord;
        while (!recordLoc.isNull() && (recordLoc.ofs != DiskLoc::NullOfs))
        {
            Record* record = (Record*)datafileManager.getView(recordLoc._a, recordLoc.ofs);
            if (record == NULL)
            {
                printf("datafile error!\n");
                return -1;
            }

            // the size of Bsonobj may not equal to the len of Record
            int len = *(reinterpret_cast<const int*>(record->_data));
            writeData(fp, record->_data, len);

            recordLoc.ofs = record->_nextOfs;
        }
    }

    fclose(fp);
    return 0;
}

