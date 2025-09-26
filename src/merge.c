#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "init.h"

// Parse "Parent: <hash>" or "Parent: none" from a commit file.
// Returns 0 with outParent set, 1 if none, -1 on error.
// parse "File N: <path> <blob>" lines into a hashtable: key=path, val=blob

static int compareFiles(FILE* f1, FILE* f2) {

}

static int read_parent_of(const char *commitHash, char *outParent, size_t outsz) {
    char path[512];
    snprintf(path, sizeof(path), ".mockgit/commits/%s", commitHash);
    FILE *cf = fopen(path, "r");
    if (!cf) { perror(path); return -1; }

    char line[512] = {0};
    if (!fgets(line, sizeof(line), cf)) { fclose(cf); return -1; }
    fclose(cf);
    strip_nl(line);

    // Expect "Parent: ..."
    const char *p = strstr(line, "Parent:");
    if (!p) return -1;
    p += 7;
    while (*p == ' ' || *p == '\t') ++p;

    if (strncmp(p, "none", 4) == 0) return 1;

    size_t n = strcspn(p, " \t\r\n");
    if (n + 1 > outsz) return -1;
    memcpy(outParent, p, n);
    outParent[n] = '\0';
    return 0;
}


//find ours, theirs and base
//ours = head's tip (the current commit from head)
//theirs = commit at the tip of the branch being merged in
//base = last common ancestor commit of ours and theirs
int merge(char *branchname) {
    // 1) Read HEAD ref, resolve to tip commit
    FILE *head = fopen(".mockgit/HEAD", "r");
    if (!head) { perror("HEAD"); return 1; }
    char headRef[512] = {0};
    if (!fgets(headRef, sizeof(headRef), head)) { fclose(head); return 1; }
    fclose(head);
    strip_nl(headRef);

    char oursTip[512] = {0};
    if (strncmp(headRef, "branches/", 9) == 0) {
        char headBranchPath[512];
        snprintf(headBranchPath, sizeof(headBranchPath), ".mockgit/%s", headRef);
        FILE *bf = fopen(headBranchPath, "r");
        if (!bf) { perror(headBranchPath); return 1; }
        if (!fgets(oursTip, sizeof(oursTip), bf)) { fclose(bf); return 1; }
        fclose(bf);
        strip_nl(oursTip);
    } else {
        snprintf(oursTip, sizeof(oursTip), "%s", headRef);
        strip_nl(oursTip);
    }

    // 2) Build ancestor set of ours
    HashTable *seen = createTable();
    if (!seen) return 1;

    char cur[512]; snprintf(cur, sizeof(cur), "%s", oursTip);
    insertItem(seen, cur, "1");
    for (;;) {
        char parentHash[512] = {0};
        int rc = read_parent_of(cur, parentHash, sizeof(parentHash));
        if (rc == 1) break;           // Parent: none
        if (rc != 0) { freeTable(seen); return 1; }
        insertItem(seen, parentHash, "1");
        snprintf(cur, sizeof(cur), "%s", parentHash);  // <-- advance!
    }

    // 3) Read target branch tip once
    char branchPath[512];
    snprintf(branchPath, sizeof(branchPath), ".mockgit/branches/%s", branchname);
    FILE *br = fopen(branchPath, "r");
    if (!br) {
        fprintf(stderr, "Error: Branch '%s' does not exist.\n", branchname);
        freeTable(seen);
        return 1;
    }
    char theirsTip[512] = {0};
    if (!fgets(theirsTip, sizeof(theirsTip), br)) { fclose(br); freeTable(seen); return 1; }
    fclose(br);
    strip_nl(theirsTip);

    // 4) Walk theirs upward until we hit an ancestor in the set
    char lca[512] = {0};
    snprintf(cur, sizeof(cur), "%s", theirsTip);
    for (;;) {
        if (searchTable(seen, cur) != NULL) {
            snprintf(lca, sizeof(lca), "%s", cur);
            break;
        }
        char parentHash[512] = {0};
        int rc = read_parent_of(cur, parentHash, sizeof(parentHash));
        if (rc == 1) { // none
            fprintf(stderr, "merge: no common ancestor found.\n");
            freeTable(seen);
            return 1;
        }
        if (rc != 0) { freeTable(seen); return 1; }
        snprintf(cur, sizeof(cur), "%s", parentHash);  // advance
    }

    printf("Last common commit (LCA) with '%s': %s\n", branchname, lca);
    freeTable(seen);

    //parse three blobs (BASE = LCA, OURS = HEAD tip, THEIRS = other branch tip)
    

    size_t pathLength = strlen(".mockgit/commits/") + 65;
    char *lcaPath = malloc(pathLength); char *oursPath = malloc(pathLength); char *theirsPath = malloc(pathLength);

    snprintf(lcaPath, pathLength, ".mockgit/commits/%s", lca);
    snprintf(oursPath, pathLength, ".mockgit/commits/%s", oursTip);
    snprintf(theirsPath, pathLength, ".mockgit/commits/%s", theirsTip);

    FILE *lcaFile = fopen(lcaPath, "r");
    FILE *oursFile = fopen(oursPath, "r");
    FILE *theirsFile = fopen(theirsPath, "r");

    if (!oursFile || !theirsFile || !lcaFile) {
        perror("Invalid file open, terminating");
        return 0;
    }

    /*
    If OURS == THEIRS → nothing to do (already the same).

    If OURS == BASE and THEIRS != BASE → take THEIRS as the result.

    If THEIRS == BASE and OURS != BASE → take OURS as the result.
    */

)

    //restore from
    return 0;
}
