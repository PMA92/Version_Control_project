#include <stdio.h>
#include <string.h>
#include "init.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: mockgit <command>\n");
        return 1;
    }
    if (strcmp(argv[1], "add") == 0)
    {
        printf("Adding file: %s\n", argv[2]);
        if (argc < 3)
        {
            printf("Usage: mockgit add <filename>\n");
            return 1;
        }
        return addFiles(argv[2]);
    }
    if (strcmp(argv[1], "init") == 0)
    {
        return makeInitFiles();
    }

    printf("Unknown command: %s\n", argv[1]);
    return 1;
}
