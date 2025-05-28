#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hashtable.h"

int commit(){
    FILE *indexFile = fopen(".mockgit/index", "r");

    HashTable *table = createTable();

    char line[256], filename[128], hash[65];

    while (fgets(line, sizeof(line), indexFile)) {
        if (sscanf(line, "%127s %64s", filename, hash) == 2) {
            insertItem(table, filename, hash);
        }
    }

    fclose(indexFile);

    
}