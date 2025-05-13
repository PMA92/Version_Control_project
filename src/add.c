#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int addFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    return 0;
}