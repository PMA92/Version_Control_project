#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char *hash(FILE *file, unsigned char *buffer){    
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
    free(content);
    
    char *hex_output = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hex_output) {
        perror("Memory allocation failed");
        exit(1);
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf((char *)&hex_output[i * 2], "%02x", buffer[i]);
    }
    hex_output[SHA256_DIGEST_LENGTH * 2] = '\0';

    return hex_output;
}   

int addFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    } 
    else{
        printf("File opened: %s\n", filename);
    }
    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];

    char *hash_hex = hash(file, hash_buffer);

    rewind(file);
    char newBlobName[256];
    sprintf(newBlobName, ".mockgit/blobs/%s", hash_hex);
    
    FILE *newBlob = fopen(newBlobName, "wb");
    
    if (!newBlob) {
        perror("Error creating blob file");
        free(hash_hex);
        return 1;
    }
    
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        fputc(c, newBlob);  
    }
    free(hash_hex); 
    fclose(newBlob);
    fclose(file);
    return 0;
}