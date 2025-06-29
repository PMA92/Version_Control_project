#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int branch(char *branchName) {
    FILE *head = fopen(".mockgit/HEAD", "r");
    if (!head) {
        perror("Error opening HEAD file");
        return 1;
    }
    char headLine[256];
    if (fgets(headLine, sizeof(headLine), head)) {
        printf("Current HEAD: %s\n", headLine);
        headLine[strcspn(headLine, "\n")] = 0;
        branchName[strcspn(branchName, "\n")] = 0; // Remove newline character if present
        if (strncmp(headLine, "./mockgit/branches", strlen("./mockgit/branches")) == 0) {
            printf("found branch file\n");
            FILE *currentBranch = fopen(headLine, "r");
            char line[256];
            char *branchHash = fgets(line, sizeof(line), currentBranch);
            if (!branchHash) {
                perror("Error reading current branch file");
                fclose(currentBranch); 
                fclose(head);
                return 1;
            }
            snprintf(branchHash, sizeof(line), "./mockgit/branches/%s", branchName);
            FILE *newBranch = fopen(branchName, "w");
            if (!newBranch) {
                perror("Error creating new branch file");
                fclose(currentBranch);
                fclose(head);
                return 1;
            }
            fprintf(newBranch, "%s", branchHash);
            fclose(newBranch);
            fclose(currentBranch);
            fclose(head);
            printf("Branch '%s' created successfully.\n", branchName);
        }
        else {
            printf("No current branch found. Creating new branch '%s'.\n", branchName);
            char filePath[256]; 
            snprintf(filePath, sizeof(filePath), "./mockgit/branches/%s", branchName);
            printf("%s", filePath);
            FILE *newBranch = fopen(filePath, "w"); 
            if (!newBranch) {
                perror("Error creating new branch file");
                fclose(head);
                return 1;
            }
            fprintf(newBranch, "%s", headLine);
            fclose(newBranch);
            fclose(head); 
            printf("Branch '%s' created successfully.\n", branchName);
            return 0;
        }
        return 0;
    }
    return 1;
}


