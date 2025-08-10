#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int starts_with(const char *s, const char *pfx) {
    return strncmp(s, pfx, strlen(pfx)) == 0;
}


static void strip_nl(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

int updateWorkingDirectory(char *headRef){
    char commitFile[512] = {0};
    /* Resolve to a commit file path */
    if (starts_with(headRef, ".mockgit/branches/")) {
        FILE *rf = fopen(headRef, "r");
        if (!rf) { fprintf(stderr, "Error: open %s\n", headRef); return 1; }
        char hash[65] = {0};
        if (!fgets(hash, sizeof hash, rf)) { fclose(rf); fprintf(stderr, "Error: empty branch ref\n"); return 1; }
        fclose(rf);
        strip_nl(hash);
        snprintf(commitFile, sizeof commitFile, ".mockgit/commits/%s", hash);
    } else if (starts_with(headRef, ".mockgit/commits/")) {
        snprintf(commitFile, sizeof commitFile, "%s", headRef);
    } else {
        /* assume raw hash */
        snprintf(commitFile, sizeof commitFile, ".mockgit/commits/%s", headRef);
    }

    FILE *currentCommit = fopen(commitFile, "r");
    if (!currentCommit) {
        fprintf(stderr, "Error: Could not open commit file at %s.\n", commitFile);
        return 1;
    }

    FILE *idx = fopen(".mockgit/index", "w");
    if (!idx) { perror("open index"); fclose(currentCommit); return 1; }

    char line[1024];
    while (fgets(line, sizeof line, currentCommit)) {
        if (strncmp(line, "File ", 5) != 0) continue;

        int num = 0;
        char filename[512], hash[128];
        if (sscanf(line, "File %d: %511s %127s", &num, filename, hash) != 3) continue;

        /* write blob -> file */
        char blobPath[512];
        snprintf(blobPath, sizeof blobPath, ".mockgit/blobs/%s", hash);

        FILE *curBlob = fopen(blobPath, "rb");
        if (!curBlob) { fprintf(stderr, "Error: open blob %s\n", blobPath); fclose(currentCommit); fclose(idx); return 1; }

        if (ensure_parent_dirs(filename) != 0) { fclose(curBlob); fclose(currentCommit); fclose(idx); return 1; }

        FILE *curFile = fopen(filename, "wb");
        if (!curFile) { perror("open out"); fclose(curBlob); fclose(currentCommit); fclose(idx); return 1; }

        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof buf, curBlob)) > 0) {
            if (fwrite(buf, 1, n, curFile) != n) { perror("write"); fclose(curBlob); fclose(curFile); fclose(currentCommit); fclose(idx); return 1; }
        }
        if (ferror(curBlob)) { perror("read"); fclose(curBlob); fclose(curFile); fclose(currentCommit); fclose(idx); return 1; }

        fclose(curBlob);
        fclose(curFile);

        /* keep index in sync with this snapshot */
        fprintf(idx, "%s %s\n", filename, hash);
    }

    fclose(currentCommit);
    fclose(idx);
    return 0;
}

int checkout(char *input){
    //finding the target branch
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), ".mockgit/branches/%s", input);
    FILE *inputFile = fopen(fullPath, "r");
    if (!inputFile) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", input);
        fullPath[0] = '\0'; 
        snprintf(fullPath, sizeof(fullPath), ".mockgit/commits/%s", input);
        inputFile = fopen(fullPath, "r"); 
        if (inputFile == NULL) {
            fprintf(stderr, "Error: Commit '%s' does not exist.\n", input);
            return 1; // Error code for branch or commit not found
        }
        else{
            FILE *head = fopen(".mockgit/HEAD", "w");
            char newHead[512];
            snprintf(newHead, sizeof(newHead), ".mockgit/commits/%s", input);
            fprintf(head, "%s", input);
            fclose(head);
            fclose(inputFile);
            printf("Switched to branch '%s'.\n", input);
            updateWorkingDirectory(newHead);
        }
    }    
    else {
        FILE *head = fopen(".mockgit/HEAD", "w");
        char newHead[512];
        snprintf(newHead, sizeof(newHead), "branches/%s", input);
        fprintf(head, "%s", newHead);
        fclose(head);
        fclose(inputFile);
        printf("Switched to branch '%s'.\n", input);
        updateWorkingDirectory(fullPath);
    }
    //implement updating working directory to match last commit of desired branch/commit
    
    return 0;
}
