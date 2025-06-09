#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "init.h"
#include "hashtable.h"
#include <dirent.h>
#include <openssl/sha.h>



int status() {
    FILE *indexFile = fopen(".mockgit/index", "r");
    if (!indexFile) {
        perror("Failed to open index file");
        return 1;
    }

    HashTable *stagedTable  = createTable();

    char line[256]; char filename[128], hash[65];
    while (fgets(line, sizeof(line), indexFile)){
        if (sscanf(line, "%127s %64s", filename, hash) == 2) {
            insertItem(stagedTable, filename, hash);
        }
    }

    char latestCommitHash[65];
    FILE *head = fopen(".mockgit/HEAD", "r");
    if (!head) {
        perror("Failed to open HEAD file");
        fclose(indexFile);
        return 1;   
    }

    if (fgets(latestCommitHash, sizeof(latestCommitHash), head) != NULL){
        latestCommitHash[strcspn(latestCommitHash, "\n")] = 0; // Remove newline character
    } else {
        perror("Failed to read HEAD file");
        fclose(indexFile);
        fclose(head);
        return 1;
    }

    char latestCommitFilePath[128];
    sprintf(latestCommitFilePath, ".mockgit/commits/%s", latestCommitHash);
    FILE *latestCommit = fopen(latestCommitFilePath, "r");

    if (!latestCommit) {
        perror("Failed to open latest commit file");
        fclose(indexFile);
        fclose(head);
        return 1;
    }

    HashTable *commitedTable = createTable();
    if (latestCommit){
        while (fgets(line, sizeof(line), latestCommit)){
            if (sscanf(line, "%127s %64s", filename, hash) == 2) {
                insertItem(commitedTable, filename, hash);
            } 
        }
    }

    DIR *workingDir = opendir(".");

    struct dirent *entry;

    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];
    unsigned char *outContent = NULL;
    long outContentLen = 0;

    while (entry = readdir(workingDir)) {
        if (entry->d_name[0] == ".") continue;
        char *filename = entry->d_name;
        char curFileHash = hashToBlob(filename, hash_buffer, &outContent, &outContentLen);

        if (searchTable(commitedTable, filename) != NULL ){
            
        }
        }
    
}

