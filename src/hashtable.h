#ifndef HASHTABLE_H
#define HASHTABLE_H

#define TABLE_SIZE 1000

typedef struct {
    char *filename;
    char *hash;
} Index;

typedef struct hashtable {
    Index **files;
    int currentTableSize;
} HashTable;

// Function declarations
HashTable *createTable();
void freeTable(HashTable *table);
int insertItem(HashTable *table, char *filename, char *blobHash);
char *searchTable(HashTable *table, char *filename);
int removeFile(HashTable *table, char *filename);

#endif