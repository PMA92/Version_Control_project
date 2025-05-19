#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char *hash(const unsigned char *filename, unsigned char *buffer){
    FILE *file = fopen((const char *)filename, "rb");
    
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
    fclose(file);
    
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
    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];

    printf(hash(filename, hash_buffer));

    if(hash(filename, hash_buffer) == NULL){
        perror("Error hashing file");
        return 1;
    }

    printf("passed");
    return 0;
}