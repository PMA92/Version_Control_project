#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TABLE_SIZE 1000

typedef struct index{
    char *filename;
    char *hash;
} Index;

typedef struct hashtable{
    Index *files[TABLE_SIZE];
} HashTable;

HashTable *createTable(){
    HashTable *table = malloc(sizeof(HashTable));
    if (!table) {
        perror("Memory allocation failed");
        exit(1);
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        table->files[i] = NULL;
    }
    return table;
}

HashTable *searchTable(HashTable *table, char *filename){
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table->files[i] != NULL && strcmp(table->files[i]->filename, filename) == 0) {
            return table->files[i];
        }
    }
    return NULL;
}

HashTable *appendFile(HashTable *table, Index *index){
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table->files[i] == NULL) {
            table->files[i] = index;
            return table;
        }
    }
}

HashTable *removeFile(HashTable *table, char *filename){
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (table->files[i] != NULL && strcmp(table->files[i]->filename, filename) == 0) {
            free(table->files[i]);
            table->files[i] = NULL;
            return table;
        }
    }
}

char *hash(FILE *file, unsigned char *buffer, unsigned char **outContent, long *outContentLen){    
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
    FILE *file = fopen(filename, "rb");
    
    Index *index = malloc(sizeof(Index));
    index->filename = filename;
    index->hash = NULL;
    if (!index) {
        perror("Memory allocation failed");
        return 1;
    }

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

    char *hash_hex = hash(file, hash_buffer, &outContent, &outContentLen);
    fclose(file);

    index->hash = hash_hex;

    HashTable *table = malloc(sizeof(HashTable));
    if (!table) {
        perror("Memory allocation failed");
        free(index);
        free(hash_hex);
        free(outContent);
        return 1;
    }




    char newBlobName[256];
    sprintf(newBlobName, ".mockgit/blobs/%s", hash_hex);
    
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
    
    HashTable *existingIndex = searchTable(table, filename);

    if (!existingIndex) {
        fopen(".mockgit/index", "a");
        //read and add the filename and hash to the index file 
        table = appendFile(table, index);
        printf("File added: %s\n", filename);
    } else {
        printf("File already exists in the index: %s\n", filename);
        free(index->hash);
        free(index);
    }  
    
    
    return 0;
}