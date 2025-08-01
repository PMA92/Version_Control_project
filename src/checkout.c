#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

int checkout(char *input){
    //finding the target branch
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), ".mockgit/branches/%s", input);
    FILE *inputFile = fopen(fullPath, "r");
    if (inputFile == NULL) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", input);
        fclose(inputFile);
        fullPath[0] = '\0'; 
        snprintf(fullPath, sizeof(fullPath), ".mockgit/commits/%s", input);
        inputFile = fopen(fullPath, "r");
        if (inputFile == NULL) {
            fprintf(stderr, "Error: Commit '%s' does not exist.\n", input);
            return 1; // Error code for branch or commit not found
        }
        else{
            FILE *head = fopen(".mockgit/HEAD", "w");
            FILE *head = fopen(".mockgit/HEAD", "w");
            char *newHead = strcat("branches/", input);
            fprintf(head, newHead);
            fclose(head);
            fclose(inputFile);
            printf("Switched to branch '%s'.\n", input);
        }
    }    
    else {
        FILE *head = fopen(".mockgit/HEAD", "w");
        char *newHead = strcat("branches/", input);
        fprintf(head, newHead);
        fclose(head);
        fclose(inputFile);
        printf("Switched to branch '%s'.\n", input);
    }


 






}
