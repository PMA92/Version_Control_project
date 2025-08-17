#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "init.h"


int merge(char *branchname) {
    //we need to get the lcc(last common commit) of the branch the user specifies, and the current head
    HashTable *commitTable = createTable();
    FILE *head = fopen(".mockgit/HEAD", "r");
    
    if (!head) {
        perror("Error opening HEAD file");
        return 1;
    }

    //read head and go down commit change storing them in a hashtable
    char commit[512];
    char headRef[512];
    fgets(headRef, sizeof(headRef), head);
    if (strncmp(headRef, "branches/", 9) == 0) {
        char headBranch[512];
        snprintf(headBranch, sizeof(headBranch), ".mockgit/%s", headRef);
        FILE *branchFile = fopen(headBranch, "r");
        if (!branchFile) {
            perror("Error opening branch file");
            fclose(head);
            return 1;
        }
        fgets(commit, sizeof(commit), branchFile);
        fclose(branchFile);
    }
    else {
        snprintf(commit, sizeof(commit), "%s", headRef);
    }
    insertItem(commitTable, commit, NULL);
    char parent[512];
    char targetLCC[512];
    while (1) {
        snprintf(parent, sizeof(parent), ".mockgit/commits/%s", commit);
        FILE *commitFile = fopen(parent, "r");
        fseek(commitFile, 8, SEEK_SET); 
        fgets(parent, sizeof(parent), commitFile);
        if (strcmp(parent, "none") == 0) {
            break;
        }
        else {
            insertItem(commitTable, parent, NULL);
            fclose(commitFile);
        }

    
    fclose(head);
    //now check the branch 
    char branchPath[512];
    snprintf(branchPath, sizeof(branchPath), ".mockgit/branches/%s", branchname);
    FILE *branchFile = fopen(branchPath, "r");
    if (!branchFile) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", branchname);
        freeTable(commitTable);
        return 1;
    }
    char branchCommit[512];
    fgets(branchCommit, sizeof(branchCommit), branchFile);
    fclose(branchFile);
    if (searchTable(commitTable, branchCommit) != NULL) {
        snprintf(targetLCC, sizeof(targetLCC), "%s", branchCommit);
        break;
    } 
    }

    return 1;
}
