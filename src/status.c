#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "init.h"
#include "hashtable.h"
#include <dirent.h>
#include <openssl/sha.h>

char *readFirstLine(FILE *f){
    char line[512];
    if (fgets(line, sizeof(line), f) != NULL){
        line[strcspn(line, "\n")] = 0; // Remove newline character
        return strdup(line);
    }
    rewind(f);
    return NULL;
}

void processFileSearch(char *thisDir, HashTable *stagedTable, HashTable *commitedTable, 
    char ***stagedFiles, size_t *stagedCount, char ***modifiedFiles, 
    size_t *modifiedCount, char ***untrackedFiles, size_t *untrackedCount, unsigned char *hash_buffer, unsigned char **outContent, long *outContentLen) {
    
    if (strncmp(thisDir, "./.mockgit", 10) == 0) {
        return; // Skip the .mockgit directory
    }
    DIR *workingDir = opendir(thisDir);
    struct dirent *entry;
    

    while ((entry = readdir(workingDir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", thisDir, entry->d_name);
        if (entry->d_type == DT_DIR) {  
            processFileSearch(fullPath, stagedTable, commitedTable, stagedFiles, stagedCount, modifiedFiles, modifiedCount, untrackedFiles, untrackedCount, hash_buffer, outContent, outContentLen);
        }
        else {
            char *curFilename = fullPath;
            if (strncmp(curFilename, "./", 2) == 0) {
                curFilename += 2;
            }
            FILE *curFile = fopen(fullPath, "rb");
            if (!curFile) {
                perror(fullPath);
                continue;
            }
            char *curFileHash = hashToBlob(curFile, hash_buffer, outContent, outContentLen);
            fclose(curFile);
            if (searchTable(stagedTable, curFilename) != NULL) {
                if (searchTable(commitedTable, curFilename) == NULL) {
                    *stagedFiles = realloc(*stagedFiles, sizeof(char*) * (*stagedCount + 1));
                    (*stagedFiles)[(*stagedCount)++] = strdup(curFilename);
                }
            }
            else if (searchTable(commitedTable, curFilename) != NULL ){  
                if (strcmp(curFileHash, searchTable(commitedTable, curFilename)) != 0){
                    *modifiedFiles = realloc(*modifiedFiles, sizeof(char*) * (*modifiedCount + 1));
                    (*modifiedFiles)[(*modifiedCount)++] = strdup(curFilename);
                }
            else {
                *untrackedFiles = realloc(*untrackedFiles, sizeof(char*) * (*untrackedCount + 1));
                (*untrackedFiles)[(*untrackedCount)++] = strdup(curFilename);
            }
        }
        }
        }
    closedir(workingDir);

}


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

    char *temp = readFirstLine(head);
    strncpy(latestCommitHash, temp, sizeof(latestCommitHash)-1);
    free(temp);
    if (latestCommitHash){
    } else {
        perror("Failed to read HEAD file");
        fclose(indexFile);
        fclose(head);
        return 1;
    }

    FILE *curBranch = fopen(latestCommitHash, "r");
    latestCommitHash[0] = '\0';
    if (curBranch){
        temp = readFirstLine(curBranch);
        strncpy(latestCommitHash, temp, sizeof(latestCommitHash)-1);
    }
    
    printf(latestCommitHash);
    fclose(curBranch);

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
            if (sscanf(line, "File %*d: %127s %64s", filename, hash) == 2) {
                insertItem(commitedTable, filename, hash);
            } 
        }
    }

    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];
    unsigned char *outContent = NULL;
    long outContentLen = 0;


    char **stagedFiles = NULL;
    char **modifiedFiles = NULL;
    char **untrackedFiles = NULL;

    size_t stagedCount = 0, modifiedCount = 0, untrackedCount = 0;
    
    
    char *currentDir = ".";

    processFileSearch(currentDir, stagedTable, commitedTable, &stagedFiles, &stagedCount, &modifiedFiles, &modifiedCount, &untrackedFiles, &untrackedCount, hash_buffer, &outContent, &outContentLen);
    
    printf("Staged files:\n");
    if (stagedCount == 0) {
        printf("\tNo staged files.\n");
    }
    else {
        for (size_t i = 0; i < stagedCount; i++) {
            printf("\n\t%s", stagedFiles[i]);
        }
    }
    printf("\n\nModified files:\n");
    if (modifiedCount == 0) {
        printf("\tNo modified files.\n");
    }
    else {
        for (size_t i = 0; i < modifiedCount; i++) {
            printf("\n\t%s", modifiedFiles[i]);
        }
    }
    printf("\n\nUntracked files:\n");
    if (untrackedCount == 0) {
        printf("\tNo untracked files.\n");
    }
    else {
        for (size_t i = 0; i < untrackedCount; i++) {
            printf("\n\t%s", untrackedFiles[i]);
        }
    }
    printf("\n");
    // Free allocated memory
    for (size_t i = 0; i < stagedCount; i++) {
        free(stagedFiles[i]);
    }
    for (size_t i = 0; i < modifiedCount; i++) {
        free(modifiedFiles[i]);
    }
    for (size_t i = 0; i < untrackedCount; i++) {
        free(untrackedFiles[i]);
    }
    freeTable(commitedTable);
    freeTable(stagedTable);
    fclose(indexFile);
    fclose(latestCommit);
    fclose(head);
    return 0;
}

