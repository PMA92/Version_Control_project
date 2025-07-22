#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

int makeInitFiles()
{
    const char *initFiles[] = {
        ".mockgit",
        ".mockgit/commits",
        ".mockgit/blobs",
    };

    for (int i = 0; i < 3; i++)
    {
        if (mkdir(initFiles[i], 0777) != 0)
        {
            perror("Error creating directory");
            return 1;
        }
    }

    FILE *head = fopen(".mockgit/HEAD", "w");
    if (head == NULL)
    {
        perror("Error creating HEAD file");
        return 1;
    }
    fprintf(head, "branches/master");
    fclose(head);
    FILE *index = fopen(".mockgit/index", "w");
    if (!index)
    {
        perror("Error creating index file");
        fclose(head);
        return 1;
    }

    fclose(index);

    printf("Initalized empty mockgit repository in .mockgit\n");
    return 0;
}