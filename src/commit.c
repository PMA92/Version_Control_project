#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hashtable.h"
#include "time.h"
#include "openssl/sha.h"
#include "init.h"

int commit(char *type, char *message) {
    FILE *indexFile = fopen(".mockgit/index", "r");
    FILE *head = fopen(".mockgit/HEAD", "r");
    
    char headPath[128];
    fgets(headPath, sizeof(headPath), head);  
    headPath[strcspn(headPath, "\n")] = 0;
    int detached = strncmp(headPath, "branches/", strlen("branches/")) != 0;
    
    printf("HEAD Path: %s\n", headPath);
    char parentHash[128] = "";
    if (detached) {
        strncpy(parentHash, headPath, sizeof(parentHash));
    } else {
        char branchPath[256];
        snprintf(branchPath, sizeof(branchPath), ".mockgit/%s", headPath);
        FILE *branchFile = fopen(branchPath, "r");
        if (branchFile) {
            fgets(parentHash, sizeof(parentHash), branchFile);
            parentHash[strcspn(parentHash, "\n")] = 0;
            fclose(branchFile);
        }
    }

    //snprintf(headPath, sizeof(headPath), ".mockgit/%s", headPath);

    if (!indexFile) {
        perror("Error opening index file");
        return 1;
    }
    if (!head) {
        perror("Error opening HEAD file");
        fclose(indexFile);
        return 1;
    }
    HashTable *table = createTable();
    if (!table) {
        perror("Error creating hash table");
        fclose(indexFile);
        fclose(head);
        return 1;
    }
    char line[256], filename[128], hash[65];

    while (fgets(line, sizeof(line), indexFile)) {
        if (sscanf(line, "%127s %64s", filename, hash) == 2) {
            insertItem(table, filename, hash);
        }
    }

    // Reset the index file
    fclose(indexFile);
    FILE *inFl = fopen(".mockgit/index", "w");
    fclose(inFl);
 
    char commitContent[4096] = "";
    if (strlen(parentHash) > 0) {
        snprintf(line, sizeof(line), "Parent: %s\n", parentHash);
    } else {
        snprintf(line, sizeof(line), "Parent: none\n");
    }
    strcat(commitContent, line);
    

    time_t currentTime = time(NULL);
    char *timeString = ctime(&currentTime);
    
    strcat(commitContent, timeString);


    if (strcmp(type, "-m") == 0 && strlen(message) > 0) {
        strcat(commitContent, message);
        strcat(commitContent, "\n");
    } else {
        fprintf(stderr, "Error: Invalid or missing commit message.\n");
        fclose(head);
        freeTable(table);
        return 1;
    }
    
    
    
    printf("Committed changes with message: %s\n", message);
    printf("Commited Files: ");
    int fileIndex = 1;
    for (int i = 0; i < TABLE_SIZE; i++) {
        unsigned char hash_buffer[SHA256_DIGEST_LENGTH];
        unsigned char *outContent = NULL;
        long outContentLen = 0;
        if (table->files[i] != NULL) {
            char currentString[256];
            FILE *curFile = fopen(table->files[i]->filename, "rb");
            if (curFile) {
                sprintf(currentString, "File %d: %s %s\n", fileIndex++, table->files[i]->filename,
                        hashToBlob(curFile, hash_buffer, &outContent, &outContentLen));
                printf("%s ", table->files[i]->filename);
                strcat(commitContent, currentString);
                fclose(curFile);
            }
        }
    }

    unsigned char hashBuf[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)commitContent, strlen(commitContent), hashBuf);

    char commitHashHex[65];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(commitHashHex + (i * 2), "%02x", hashBuf[i]);
    }


    char commitFilePath[128];
    snprintf(commitFilePath, sizeof(commitFilePath), ".mockgit/commits/%s", commitHashHex);

    FILE *commitFile = fopen(commitFilePath, "w");
    if (!commitFile) {
        perror("Failed to create commit file");
        fclose(head);
        freeTable(table);
        return 1;
    }
    fprintf(commitFile, "%s", commitContent);
    fclose(commitFile);
    
    // Update HEAD or branch
    if (detached) {
        fseek(head, 0, SEEK_SET);
        ftruncate(fileno(head), 0);
        fprintf(head, "%s\n", commitHashHex);
    } else {
        char branchPath[256];
        snprintf(branchPath, sizeof(branchPath), ".mockgit/%s", headPath);
        printf("Updating branch file: %s\n", branchPath);
        FILE *branchFile = fopen(branchPath, "w");
        if (!branchFile) {
            perror("Failed to update current branch file");
            fclose(head);
            freeTable(table);
            return 1;
        }
        fprintf(branchFile, "%s\n", commitHashHex);
        fclose(branchFile);
    }

    fclose(head);
    freeTable(table);
    return 0;
}