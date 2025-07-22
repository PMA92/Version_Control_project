#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int branch(char *branchName) {
    FILE *head = fopen(".mockgit/HEAD", "r+");
    if (!head) {
        perror("Error opening HEAD file");
        return 1;
    }
    char headLine[256];
    if (fgets(headLine, sizeof(headLine), head)) {
        char filePath[256];
        headLine[strcspn(headLine, "\n")] = 0;
        branchName[strcspn(branchName, "\n")] = 0; // Remove newline character if present
        printf("headLine: %s", headLine);
        if (strncmp(headLine, "branches/", 9) == 0) {
            char currentBranchPath[256];
            snprintf(currentBranchPath, sizeof(currentBranchPath), ".mockgit/%s", headLine);
            FILE *currentBranch = fopen(currentBranchPath, "r");
            char line[256];
            char *branchHash = fgets(line, sizeof(line), currentBranch);
            if (!branchHash) {
                fprintf(stderr, "Error: Current branch has no commit hash.\n");
                fclose(currentBranch);
                fclose(head);
                return 1;
            }
            branchHash[strcspn(branchHash, "\n")] = 0; // Remove
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
        }
        else {
            // HEAD is assumed to be a raw commit hash
            char commitHash[256];
            strncpy(commitHash, headLine, sizeof(commitHash));
            commitHash[sizeof(commitHash) - 1] = '\0';  // Ensure null termination

            // Create new branch file
            snprintf(filePath, sizeof(filePath), "./.mockgit/branches/%s", branchName);
            FILE *newBranch = fopen(filePath, "w");
            if (!newBranch) {
                perror("Error creating new branch file");
                fclose(head);
                return 1;
            }
            fprintf(newBranch, "%s\n", commitHash);
            fclose(newBranch);
        
            // Update HEAD to point to new branch
            fseek(head, 0, SEEK_SET);
            ftruncate(fileno(head), 0);
            fprintf(head, "branches/%s\n", branchName);
        
            printf("Branch '%s' created successfully from detached HEAD.\n", branchName);
        }       
        fclose(head);
        return 0;

    }
    return 1;
}


