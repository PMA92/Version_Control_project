#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "init.h"

// Parse "Parent: <hash>" or "Parent: none" from a commit file.
// Returns 0 with outParent set, 1 if none, -1 on error.
// parse "File N: <path> <blob>" lines into a hashtable: key=path, val=blob


//making a dictionary for the sake of keeping files to merge with their hashes iterable
//only need a stored structure for the sake of iterability


typedef struct Vector{
    char **data;
    size_t len, capacity;
} Vector;

void vec_init(Vector *v){ 
    v->data=NULL; 
    v->len=0; 
    v->capacity=0; 
}

void push(Vector *vec, char *val){
    if (vec->len == vec->capacity){
        vec->capacity = vec->capacity ? vec->capacity * 2 : 4;
    }
    vec->data = realloc(vec->data, sizeof(char*) * vec->capacity);
    if (!vec->data) { perror("realloc"); exit(1); }
    vec->data[vec->len++] = strdup(val);
}

static void movePathsToVector(HashTable *table, Vector *out){
    if (!table) return;
    printTable(table);
    int i=0;
    int count = 0;
    while (count < table->currentTableSize || i < TABLE_SIZE){
        if (table->files[i] && table->files[i]->filename) {
            push(out, table->files[i]->filename);
            count++;
        }
        i++;
    }
}

static void freeVector(Vector *v){
    for (size_t i=0;i<v->len;i++) free(v->data[i]);
    free(v->data); v->data=NULL; v->len=v->capacity=0;
}

static int cmp_cstr(const void *a, const void *b){
    const char *sa = *(char* const*)a;
    const char *sb = *(char* const*)b;
    return strcmp(sa, sb);
}

static size_t sortVector(Vector *v){
    if (!v || v->len == 0) return 0;
    qsort(v->data, v->len, sizeof(char*), cmp_cstr);
    size_t dups = 1;
    for (size_t i=1; i<v->len; ++i){
        if (strcmp(v->data[i], v->data[dups-1])){
            v->data[dups++] = v->data[i];
        } else {
            free(v->data[i]);
        }
    }
    v->len = dups;
    return v->len;
}



static int write_blob_to_working(const char *workPath, const char *blobHash){
    if (!workPath || !blobHash) return -1;
    char blobPath[512];
    snprintf(blobPath, sizeof blobPath, ".mockgit/blobs/%s", blobHash);

    FILE *in  = fopen(blobPath, "rb");
    FILE *out = fopen(workPath, "wb");
    if (!in || !out){
        perror("open blob/work");
        if (in) fclose(in);
        if (out) fclose(out);
        return -1;
    }
    char buf[8192];
    size_t n;
    while ((n = fread(buf,1,sizeof buf,in)) > 0){
        if (fwrite(buf,1,n,out) != n) { perror("write"); fclose(in); fclose(out); return -1; }
    }
    fclose(in); fclose(out);
    return 0;
}

static HashTable* filesToMerge(FILE *commitFile){
    rewind(commitFile);
    char line[512];
    HashTable *blobAndHash = createTable();
    while (fgets(line, sizeof(line), commitFile)) {
        if (strncmp(line, "File ", 5) == 0){
            int num = 0;
            char filename[512], hash[128];
            if (sscanf(line, "File %d: %511s %127s", &num, filename, hash) == 3) {
                printf("File %d: %s %s\n", num, filename, hash);
                insertItem(blobAndHash, filename, hash);
                num++;
            }
        }
    }
    return blobAndHash;
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

    freeTable(seen);

    //parse three blobs (BASE = LCA, OURS = HEAD tip, THEIRS = other branch tip)

    size_t pathLength = strlen(".mockgit/commits/") + 65;
    char *lcaPath = malloc(pathLength); char *oursPath = malloc(pathLength); char *theirsPath = malloc(pathLength);
    
    snprintf(lcaPath, pathLength, ".mockgit/commits/%s", lca);
    snprintf(oursPath, pathLength, ".mockgit/commits/%s", oursTip);
    snprintf(theirsPath, pathLength, ".mockgit/commits/%s", theirsTip);
    
    FILE *lcaCommit = fopen(lcaPath, "r");
    FILE *oursCommit = fopen(oursPath, "r");
    FILE *theirsCommit = fopen(theirsPath, "r");
    
    
    if (!oursCommit || !theirsCommit || !lcaCommit) {
        perror("Invalid file open, terminating");
        fclose(lcaCommit); fclose(oursCommit); fclose(theirsCommit);
        free(lcaPath); free(oursPath); free(theirsPath);
        return 1;
    }

    /*
    If OURS == THEIRS → nothing to do (already the same).

    If OURS == BASE and THEIRS != BASE → take THEIRS as the result.

    If THEIRS == BASE and OURS != BASE → take OURS as the result.
    */
    HashTable *lcaTbl = filesToMerge(lcaCommit);
    HashTable *oursTbl = filesToMerge(oursCommit);
    HashTable *theirsTbl = filesToMerge(theirsCommit);

    Vector paths;
    vec_init(&paths);
    movePathsToVector(lcaTbl, &paths);
    movePathsToVector(oursTbl, &paths);
    movePathsToVector(theirsTbl, &paths);
    sortVector(&paths);

    printf("files to go through: %zu\n", paths.len);
    for (size_t i=0; i<paths.len; ++i){
        char *curPath = paths.data[i];
        char *lcaHash = searchTable(lcaTbl, curPath);
        char *oursHash = searchTable(oursTbl, curPath);
        char *theirsHash = searchTable(theirsTbl, curPath);


        int sameBaseAndBranch = (lcaHash && theirsHash) ? strcmp(lcaHash, theirsHash) == 0 : (lcaHash == theirsHash);
        int sameMainAndBase = (lcaHash && oursHash) ? strcmp(lcaHash, oursHash) == 0 : (lcaHash == oursHash);
        int sameMainAndBranch = (theirsHash && oursHash) ? strcmp(theirsHash, oursHash) == 0 : (theirsHash == oursHash);        

        if (sameMainAndBranch) {
            // nothing to do
            printf("Branch is the same as main, nothing to merge");
        } else if (sameMainAndBase && !sameBaseAndBranch) {
            // take theirs
            printf("mananbase and base and branch");
            if (theirsHash){
                printf("Merging %s from branch %s into current branch\n", curPath, branchname);
                write_blob_to_working(curPath, theirsHash);
            }
        } else if (sameBaseAndBranch && !sameMainAndBase) {
            // take ours
            printf("base and branch and not main and base");
            if (oursHash){
                printf("Keeping %s from current branch\n", curPath);
                write_blob_to_working(curPath, oursHash);
            }
        } else {
            // merge conflict
            printf("Merge conflict in %s\n", curPath);
            return 1;
        }
    }
    
    head = fopen(".mockgit/HEAD", "w");
    fprintf(head, "branches/master");
    fclose(head);
    remove(branchPath);

    fclose(lcaCommit);
    fclose(oursCommit);
    fclose(theirsCommit);
    free(lcaPath); free(oursPath); free(theirsPath);
    //restore from
    freeVector(&paths);
    return 0;   
}
