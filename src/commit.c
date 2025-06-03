#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hashtable.h"
#include "time.h"
#include "openssl/sha.h"

int commit(char *type, char *message) {
    FILE *indexFile = fopen(".mockgit/index", "r");
    FILE *head = fopen(".mockgit/HEAD", "r+");
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
    
    fseek(head, 0, SEEK_END);
    long size = ftell(head);

    char parentHash[65] = "";
    char parentLine[128];
    if (size != 0) {
        fseek(head, 0, SEEK_SET);
        fgets(parentHash, sizeof(parentHash), head);
        snprintf(parentLine, sizeof(parentLine), "Parent: %s", parentHash);
    } else {
        snprintf(parentLine, sizeof(parentLine), "Parent: none");
    }
    strcat(commitContent, parentLine);
    strcat(commitContent, "\n");

    time_t currentTime = time(NULL);
    char *timeString = ctime(&currentTime);
    
    strcat(commitContent, timeString);


    if (strcmp(type, "-m") == 0) {
        if (strcmp(message, "") == 0) {
            fprintf(stderr, "Error: Commit message cannot be empty.\n");
            freeTable(table);
            return 1;
        
        } else {
            strcat(commitContent, message);
            strcat(commitContent, "\n");
        }
    } else {
        perror("Invalid commit type. Use -m for message.");
    }
    printf("Committed changes with message: %s\n", message);
    printf("Commited Files: ");
    int fileIndex = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table->files[i] != NULL) {
            char currentString[256];
            sprintf(currentString, "File %d: %s\n", fileIndex++, table->files[i]->filename);
            printf("%s ", table->files[i]->filename);
            strcat(commitContent, currentString);
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
        return 1;
    }
    fprintf(commitFile, "%s", commitContent);
    fclose(commitFile);

    fseek(head, 0, SEEK_SET);
    ftruncate(fileno(head), 0);
    
    fprintf(head, "%s", commitHashHex);
    fclose(head);
    freeTable(table);
    return 0;
}