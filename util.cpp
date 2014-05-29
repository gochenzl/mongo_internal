#include <stdio.h>
#include <string.h>
#include <string>
#include "pdfile.h"
#include "mmap.h"

unsigned long long getFileSize(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
        return -1;

    if (fseek(fp, 0, SEEK_END) != 0)
        return -1;

    long long size = ftell(fp);

    fclose(fp);

    return size;
}

int NamespaceHash(const char* p)
{
    unsigned x = 0;
    while (*p) {
        x = x * 131 + *p;
        p++;
    }

    return (x & 0x7fffffff) | 0x8000000; // must be > 0
}

bool findNamespaceDetails(const std::string& nsFileName, const std::string& collname, NamespaceDetails& details)
{
    unsigned long long filesize = getFileSize(nsFileName.c_str());
    if (filesize < 0)
        return false;

    MemoryMappedFile mmfile(nsFileName.c_str());
    void* view = mmfile.map();
    if (view == NULL)
        return false;

    Node* nodes = (Node*)view;

    int h = NamespaceHash(collname.c_str());
    int n = static_cast<int>(filesize / sizeof(Node));
    int i = h % n;
    int start = i;
    int chain = 0;
    int maxChain = (int) (n * 0.05);
    while (1)
    {
        if ((nodes[i].hash == h) && (strcmp(nodes[i].k.buf, collname.c_str()) == 0))
        {
            details = nodes[i].value;
            return true;
        }

        i = (i+1) % n;
        chain++;
        if ((i == start) || (chain >= maxChain))
            break;
    }

    return false;
}
