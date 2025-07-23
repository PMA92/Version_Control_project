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
    if (strcmp(argv[1], "checkout") == 0){
        return checkout(argv[2]);
    }
    if (strcmp(argv[1], "branch") == 0){
        return branch(argv[2]);
    }
    if (strcmp(argv[1], "status") == 0)
    {
        return status();
    }
    if (strcmp(argv[1], "log") == 0){
        return logCommits();
    }
    else if (strcmp(argv[1], "commit") == 0){
        return commit(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "add") == 0)
    {
        printf("Adding file: %s\n", argv[2]);
        if (argc < 3)
        {
            printf("Usage: mockgit add <filename>\n");
            return 1;
        }
        return addFiles(argc - 2, &argv[2]);
    }
    else if (strcmp(argv[1], "init") == 0)
    {
        return makeInitFiles();
    }

    printf("Unknown command: %s\n", argv[1]);
    return 1;
}
