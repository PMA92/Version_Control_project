#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

int checkout(char *input){
    DIR *branches = opendir(".mockgit/branches")

    char *target[256]
    struct dirent *entry
    while(entry = readdir(branches)){
        char branchPath[512];
        snprintf(branchPath, sizeof(branchPath), "%s/%s", branches, entry->d_name);
        char *currentBranch = fopen(branchPath, "r")
        if (currentBranch != NULL){
            fgets(target, sizeof(target), currentBranch)
        }
    }
}
