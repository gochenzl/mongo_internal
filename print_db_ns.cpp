#include <stdio.h>
#include "pdfile.h"
#include "mmap.h"
#include "util.h"

int main(int argc, char* argv[])
{
    printf("hash node size = %d\n\n", sizeof(Node));

    if (argc < 2)
    {
        printf("Usage: print_db_ns db_ns_filename\n");
        return -1;
    }

    unsigned long long size = getFileSize(argv[1]);
    if (size < 0)
        return -1;

    MemoryMappedFile mmfile(argv[1]);
    void* view = mmfile.map();
    if (view == NULL)
        return -1;

    Node* nodes = (Node*)view;
    int n = static_cast<int>(size / sizeof(Node));
    for (int i = 0; i < n; i++)
    {
        if (nodes[i].hash == 0)
            continue;

        printf("name = %s\n", nodes[i].k.buf);
        printf("hash = %d\n", nodes[i].hash);
        printf("------------------\n");
    }

    return 0;
}

