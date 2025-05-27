#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define TABLE_SIZE 1000

typedef struct {
    char *filename;
    char *hash;
} Index;

typedef struct hashtable{
    Index **files;
    int currentTableSize;
} HashTable;

unsigned int hash(const char *key) {
    unsigned int hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % TABLE_SIZE;
}

HashTable *createTable(){
    HashTable *table = malloc(sizeof(HashTable));
    if (!table) {
        perror("Memory allocation failed at creating table");
        exit(1);
    }
    table->files = calloc(TABLE_SIZE, sizeof(Index *));
    if (!table->files) {
        perror("Memory allocation failed at sizing files array");
        free(table);
        exit(1);
    }
    table->currentTableSize = 0;
    return table;
}

void freeTable(HashTable *table){
    if (table == NULL) {
        return;
    }
    if (table->files != NULL) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            if (table->files[i] != NULL) {
                free(table->files[i]->filename);
                free(table->files[i]->hash);
                free(table->files[i]);
            }
        }
        free(table->files);
    }
    free(table);
}

char *searchTable(HashTable *table, char *filename){
    if (table == NULL || table->files == NULL) {
        fprintf(stderr, "Table is not initialized.\n");
        return NULL;
    }
    unsigned int tableHash = hash(filename);
    if(table->files[tableHash] == NULL) {
        fprintf(stderr, "File not found in table.\n");
        return NULL;
    }
    if (strcmp(table->files[tableHash]->filename, filename) == 0) {
        return table->files[tableHash]->filename;
    }
    return NULL;
}

int insertItem(HashTable *table, char *filename, char *blobHash) {
    unsigned int tableHash = hash(filename);
    
    Index *newFile = malloc(sizeof(Index));
    if (!newFile) {
        perror("Memory allocation failed at inserting item");
        return 1;
    }
    newFile->filename = strdup(filename);
    newFile->hash = strdup(blobHash);
    
    if (!newFile->filename || !newFile->hash) {
        perror("Memory allocation failed at duplicating filename or hash");
        free(newFile->filename);
        free(newFile->hash);
        free(newFile);
        return 1;
    }

    if (newFile->filename == NULL) {
        free(newFile);
        return 0;
    }

    if (newFile->hash == NULL) {
        free(newFile->filename);
        free(newFile->hash);
        return 0;
    }

    if (table->files[tableHash] != NULL) {
        free(table->files[tableHash]->filename);
        free(table->files[tableHash]->hash);
        free(table->files[tableHash]);
    }

    table->files[tableHash] = newFile;
    table->currentTableSize++;
    return 0;
}

int removeFile(HashTable *table, char *filename){
    if (table == NULL || table->files == NULL) {
        fprintf(stderr, "Table is not initialized.\n");
        exit(1);
    }

    unsigned int tableHash = hash(filename);
    if (table->files[tableHash] != NULL){
        if (strcmp(table->files[tableHash]->filename, filename) == 0) {
            free(table->files[tableHash]->filename);
            free(table->files[tableHash]->hash);
            free(table->files[tableHash]);
            table->files[tableHash] = NULL;
            table->currentTableSize--;
            return 1;
        }
    }
    return 0;
}

char *hashToBlob(FILE *file, unsigned char *buffer, unsigned char **outContent, long *outContentLen){    
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long filelen = ftell(file);
    rewind(file);

    
    unsigned char *content = malloc(filelen);
    if (!content) {
        perror("Memory allocation failed");
        fclose(file);
        exit(1);
    }

    if (fread(content, 1, filelen, file) != filelen) {
        perror("Failed to read file");
        free(content);
        fclose(file);
        exit(1);
    }
    
    SHA256(content, filelen, buffer);
    
    char *hex_output = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hex_output) {
        perror("Memory allocation failed");
        exit(1);
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf((char *)&hex_output[i * 2], "%02x", buffer[i]);
    }
    hex_output[SHA256_DIGEST_LENGTH * 2] = '\0';

    *outContent = content;
    *outContentLen = filelen;

    return hex_output;
}   

int addFile(char *filename)
{
    // blobbing
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("Error opening file");
        return 1;
    } 
    else{
        printf("File opened: %s\n", filename);
    }
    
    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];

    unsigned char *outContent = NULL;
    long outContentLen = 0;

    char *hash_hex = hashToBlob(file, hash_buffer, &outContent, &outContentLen);
    fclose(file);



    char newBlobName[256];
    sprintf(newBlobName, ".mockgit/blobs/%s", hash_hex);
    
    if (access(newBlobName, F_OK) == 0) {
        printf("Blob already exists: %s\n", newBlobName);
    } else {
        FILE *newBlob = fopen(newBlobName, "wb");
    // write contents as before
    }   

    FILE *newBlob = fopen(newBlobName, "wb");
    
    if (!newBlob) {
        perror("Error creating blob file");
        free(hash_hex);
        return 1;
    }
    
    if (fwrite(outContent, 1, outContentLen, newBlob) != outContentLen) {
        perror("Error writing to blob file");
        fclose(newBlob);
        free(hash_hex);
        free(outContent);
        return 1;
    }
    
    //indexing
    FILE *indexFile = fopen(".mockgit/index", "r+");   

    HashTable *table = createTable();
    
    if (!indexFile) {
        perror("Error opening index file");
        exit(1);
    }
    else {
        printf("Index file opened successfully.\n");
    }

    char line[256];
    char currentLineFilename[128];
    char currentLineHash[65];



    while (fgets(line, sizeof(line), indexFile)) {
        if (sscanf(line, "%s %s", currentLineFilename, currentLineHash) == 2) {
            insertItem(table, currentLineFilename, currentLineHash);
        }
    }


    rewind(indexFile);
    insertItem(table, filename, hash_hex);
    

    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table->files[i] != NULL) {
            fwrite(table->files[i]->filename, 1, strlen(table->files[i]->filename), indexFile);
            fwrite(" ", 1, 1, indexFile);
            fwrite(table->files[i]->hash, 1, strlen(table->files[i]->hash), indexFile);
            fwrite("\n", 1, 1, indexFile);
        }
    }
    fclose(newBlob);
    fclose(indexFile);

    freeTable(table);
    return 0;

}