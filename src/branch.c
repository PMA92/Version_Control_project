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
        char filePath[256];
        headLine[strcspn(headLine, "\n")] = 0;
        branchName[strcspn(branchName, "\n")] = 0; // Remove newline character if present
        if (strncmp(headLine, "./.mockgit/branches", strlen("./.mockgit/branches")) == 0) {
            FILE *currentBranch = fopen(headLine, "r");
            char line[256];
            char *branchHash = fgets(line, sizeof(line), currentBranch);
            if (!branchHash) {
                perror("Error reading current branch file");
                fclose(currentBranch); 
                fclose(head);
                return 1;
            }
            snprintf(filePath, sizeof(filePath), "./.mockgit/branches/%s", branchName);
            FILE *newBranch = fopen(filePath, "w");
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
            snprintf(filePath, sizeof(filePath), "./.mockgit/branches/%s", branchName);
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


