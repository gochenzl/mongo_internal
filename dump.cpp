#include <stdio.h>
#include <string>
#include "pdfile.h"
#include "mmap.h"
#include "util.h"
#include "datafile_manager.h"

static void writeData(FILE* fp, const char* buf, size_t size)
{
    size_t toWrite = size;
    size_t written = 0;

    while (toWrite) {
        size_t ret = fwrite(buf+written, 1, toWrite, fp);
        toWrite -= ret;
        written += ret;
    }
}

// dump all record from specified collection
// output file is collname.bson, it can be used by mongoresotre
// Usage: ./dump dbpath dbname collname
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        printf("Usage: ./dump dbpath dbname collname\n");
        return -1;
    }

    std::string dbpath(argv[1]);
    std::string dbname(argv[2]);
    std::string collname = dbname + "." + argv[3];

    std::string nsFileName = dbpath + "/" + dbname + ".ns";
    NamespaceDetails details;
    if (!findNamespaceDetails(nsFileName, collname, details))
        return -1;

    std::string bsonFilename = std::string(argv[3]) + ".bson";
    FILE* fp = fopen(bsonFilename.c_str(), "wb");

    DatafileManager datafileManager(dbpath, dbname);
    DiskLoc loc = details._firstExtent;
    while (!loc.isNull())
    {
        Extent* extent = (Extent*)datafileManager.getView(loc._a, loc.ofs);
        if (extent == NULL)
        {
            printf("datafile error!\n");
            return -1;
        }

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

        loc = extent->xnext;
    }
    
    fclose(fp);
    return 0;
}
