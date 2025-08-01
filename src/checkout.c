#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

char *findContent(char *directoryName, char *input)
    snprintf(directoryName, sizeof(directoryName), ".mockgit/%s", directoryName)
    DIR *directory = opendir(directoryName);

    char *checkoutResult;
    struct dirent *entry;
    while(entry = readdir(directoryName)){
        char *target;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", directoryName, entry->d_name);
        char *currentFile = fopen(path, "r");
        if (currentFile != NULL){
            fgets(target, sizeof(target), currentFile);
            if (strcmp(target, strcat("branches/", input))){
                checkoutResult = target
                break;
            }                                                                                                                         
        }
    }
    return checkoutBranch
int checkout(char *input){
    //finding the target branch
    char *checkoutBranch = findContent("branches", input)
    //check if commit hash
    if (strlen(checkoutBranch) == 0){
        char *checkoutCommit = findContent("commits", input)
    }






}
