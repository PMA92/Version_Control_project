#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include "init.h"
#include "hashtable.h"

/* Helper: read first non-empty line, returns malloc'd string or NULL.
   Caller must free. */
char *readFirstLine(FILE *f){
    if (!f) return NULL;
    char line[1024];
    rewind(f);
    while (fgets(line, sizeof(line), f)) {
        // trim newline
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] != '\0') return strdup(line);
    }
    rewind(f);
    return NULL;
}

/* Helper: returns true if path refers to or is inside .mockgit, .git, or ./src */
static int should_skip_dir(const char *path) {
    if (!path) return 0;
    if (strcmp(path, ".mockgit") == 0 || strcmp(path, "./.mockgit") == 0) return 1;
    if (strcmp(path, ".git") == 0 || strcmp(path, "./.git") == 0) return 1;
    if (strcmp(path, "src") == 0 || strcmp(path, "./src") == 0) return 1;
    if (strcmp(path, "Compile") == 0 || strcmp(path, "./src") == 0) return 1;
    if (strcmp(path, "README") == 0) return 1;
    if (strcmp(path, "") == 0) return 1;
    if (strcmp(path, "mockgit") == 0) return 1;
    /* skip if path contains /.mockgit or /.git anywhere */
    if (strstr(path, "/.mockgit") != NULL) return 1;
    if (strstr(path, "/.git") != NULL) return 1;
    if (strstr(path, ".vscode") != NULL) return 1;

    return 0;
}

/* Recursively walk working tree starting at thisDir (e.g. ".") and populate lists.
   outContent/outContentLen are forwarded to hashToBlob; hash_buffer is reuse buffer. */
void processFileSearch(const char *thisDir, HashTable *stagedTable, HashTable *commitedTable,
    char ***stagedFiles, size_t *stagedCount, char ***modifiedFiles, size_t *modifiedCount,
    char ***untrackedFiles, size_t *untrackedCount,
    unsigned char *hash_buffer, unsigned char **outContent, long *outContentLen) {

    if (should_skip_dir(thisDir)) return;

    DIR *workingDir = opendir(thisDir);
    if (!workingDir) {
        // perror is useful during debugging
        // but don't abort entire run if a subdir can't be opened
        // perror(thisDir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(workingDir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", thisDir, entry->d_name);

        struct stat st;
        if (stat(fullPath, &st) == -1) {
            // can't stat, skip
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // recurse into directory
            processFileSearch(fullPath, stagedTable, commitedTable, stagedFiles, stagedCount,
                              modifiedFiles, modifiedCount, untrackedFiles, untrackedCount,
                              hash_buffer, outContent, outContentLen);
            continue;
        }

        /* It's a file: compute working tree hash and compare to staged/committed */
        FILE *curFile = fopen(fullPath, "rb");
        if (!curFile) {
            // file disappeared or unreadable; skip
            continue;
        }

        char *curFileHash = hashToBlob(curFile, hash_buffer, outContent, outContentLen);
        fclose(curFile);
        if (!curFileHash) continue;

        /* We want the filename relative to repository root (e.g. "./foo" -> "foo").
           entry->d_name only has the basename; if thisDir is "." then use basename,
           else include relative dir: thisDir/entry->d_name */
        char relName[1024];
        if (strcmp(thisDir, ".") == 0) snprintf(relName, sizeof(relName), "%s", entry->d_name);
        else {
            /* drop leading "./" if present */
            if (strncmp(thisDir, "./", 2) == 0)
                snprintf(relName, sizeof(relName), "%s/%s", thisDir + 2, entry->d_name);
            else
                snprintf(relName, sizeof(relName), "%s/%s", thisDir, entry->d_name);
        }

        char *stagedHash = searchTable(stagedTable, relName);
        char *committedHash = searchTable(commitedTable, relName);

        if (stagedHash && !committedHash) {
            // staged but not in commit -> staged new file
            *stagedFiles = realloc(*stagedFiles, sizeof(char*) * (*stagedCount + 1));
            (*stagedFiles)[(*stagedCount)++] = strdup(relName);
        }
        else if (stagedHash && committedHash) {
            // tracked, staged and committed: compare working tree -> staged (if working changed since stage),
            // and staged -> committed (if staged differs from committed should be flagged too).
            if (strcmp(curFileHash, stagedHash) != 0) {
                // working copy changed since staged
                *modifiedFiles = realloc(*modifiedFiles, sizeof(char*) * (*modifiedCount + 1));
                (*modifiedFiles)[(*modifiedCount)++] = strdup(relName);
            } else if (strcmp(stagedHash, committedHash) != 0) {
                // staged differs from committed (but working equals staged) -> still "modified" relative to commit
                *modifiedFiles = realloc(*modifiedFiles, sizeof(char*) * (*modifiedCount + 1));
                (*modifiedFiles)[(*modifiedCount)++] = strdup(relName);
            }
        }
        else if (!stagedHash && committedHash) {
            // tracked in commit but not staged. If working tree differs from commit -> modified.
            if (strcmp(curFileHash, committedHash) != 0) {
                *modifiedFiles = realloc(*modifiedFiles, sizeof(char*) * (*modifiedCount + 1));
                (*modifiedFiles)[(*modifiedCount)++] = strdup(relName);
            }
        }
        else {
            // neither staged nor committed -> untracked
            *untrackedFiles = realloc(*untrackedFiles, sizeof(char*) * (*untrackedCount + 1));
            (*untrackedFiles)[(*untrackedCount)++] = strdup(relName);
        }

        free(curFileHash);
    }

    closedir(workingDir);
}

/* status(): read index, HEAD -> branch -> commit, read commit file, walk working tree and print statuses */
int status(void) {
    FILE *indexFile = fopen(".mockgit/index", "r");
    if (!indexFile) {
        perror("Failed to open index file");
        return 1;
    }

    HashTable *stagedTable  = createTable();

    char line[1024];
    char filename[512], hash[256];
    while (fgets(line, sizeof(line), indexFile)) {
        if (sscanf(line, "%511s %255s", filename, hash) == 2) {
            insertItem(stagedTable, filename, strdup(hash));
        }
    }

    /* Read HEAD (branch name or commit hash) */
    FILE *head = fopen(".mockgit/HEAD", "r");
    if (!head) {
        perror("Failed to open HEAD file");
        freeTable(stagedTable);
        fclose(indexFile);
        return 1;
    }

    char *headLine = readFirstLine(head);
    if (!headLine) {
        fprintf(stderr, "Empty HEAD file\n");
        freeTable(stagedTable);
        fclose(indexFile);
        fclose(head);
        return 1;
    }

    /* Determine commit hash: HEAD may contain branch name; branch file is at .mockgit/<branch>
       The branch file may contain the commit hash or a filename under .mockgit/commits/. */
    char commitHash[65] = {0};
    char branchPath[512];
    snprintf(branchPath, sizeof(branchPath), ".mockgit/%s", headLine);
    FILE *branchFile = fopen(branchPath, "r");

    if (branchFile) {
        char *bline = readFirstLine(branchFile);
        if (bline) {
            // bline may be a 64-char hash or a commit filename. If it's exactly 64 hex chars treat as hash.
            if (strlen(bline) == 64) {
                strncpy(commitHash, bline, sizeof(commitHash)-1);
            } else {
                // assume it names a file under .mockgit/commits/
                char attempt[512];
                snprintf(attempt, sizeof(attempt), ".mockgit/commits/%s", bline);
                // trim newline already done by readFirstLine
                FILE *tryC = fopen(attempt, "r");
                if (tryC) {
                    // this opens the commit file; treat commit name as bline
                    strncpy(commitHash, bline, sizeof(commitHash)-1);
                    fclose(tryC);
                } else {
                    // maybe the branch file simply contains the hash but had different length; copy best-effort
                    strncpy(commitHash, bline, sizeof(commitHash)-1);
                }
            }
            free(bline);
        }
        fclose(branchFile);
    } else {
        /* No branch file at .mockgit/<headLine>, maybe HEAD already contains a commit hash */
        if (strlen(headLine) == 64) {
            strncpy(commitHash, headLine, sizeof(commitHash)-1);
        } else {
            // fall back: user didn't give a recognized branch or commit hash
            fprintf(stderr, "Can't resolve branch/commit from HEAD: %s\n", headLine);
            free(headLine);
            freeTable(stagedTable);
            fclose(indexFile);
            fclose(head);
            return 1;
        }
    }
    free(headLine);
    fclose(head);

    /* Open commit file under .mockgit/commits/<commitHash> */
    char commitFilePath[512];
    snprintf(commitFilePath, sizeof(commitFilePath), ".mockgit/commits/%s", commitHash);
    FILE *currentCommit = fopen(commitFilePath, "r");
    if (!currentCommit) {
        /* Try fallback: maybe commitHash is actually a file under .mockgit/<commitHash> that contains
           the name of the commit file (legacy layout) */
        char altPath[512];
        snprintf(altPath, sizeof(altPath), ".mockgit/%s", commitHash);
        FILE *alt = fopen(altPath, "r");
        if (alt) {
            char *altline = readFirstLine(alt);
            if (altline) {
                snprintf(commitFilePath, sizeof(commitFilePath), ".mockgit/commits/%s", altline);
                free(altline);
                currentCommit = fopen(commitFilePath, "r");
            }
            fclose(alt);
        }
    }

    if (!currentCommit) {
        fprintf(stderr, "Failed to open commit file for hash: %s\n", commitHash);
        freeTable(stagedTable);
        fclose(indexFile);
        return 1;
    }

    /* Parse commit file for tracked files and hashes. Expect lines like:
         File 0: <filename> <hash>
       Adapt sscanf format to match your commit format.
    */
    HashTable *commitedTable = createTable();
    while (fgets(line, sizeof(line), currentCommit)) {
        if (sscanf(line, "File %*d: %511s %255s", filename, hash) == 2) {
            insertItem(commitedTable, strdup(filename), strdup(hash));
        }
    }
    fclose(currentCommit);

    unsigned char hash_buffer[SHA256_DIGEST_LENGTH];
    unsigned char *outContent = NULL;
    long outContentLen = 0;

    char **stagedFiles = NULL;
    char **modifiedFiles = NULL;
    char **untrackedFiles = NULL;
    size_t stagedCount = 0, modifiedCount = 0, untrackedCount = 0;

    processFileSearch(".", stagedTable, commitedTable,
                      &stagedFiles, &stagedCount,
                      &modifiedFiles, &modifiedCount,
                      &untrackedFiles, &untrackedCount,
                      hash_buffer, &outContent, &outContentLen);

    printf("Staged files:\n");
    if (stagedCount == 0) {
        printf("\tNo staged files.\n");
    } else {
        for (size_t i = 0; i < stagedCount; ++i) printf("\t%s\n", stagedFiles[i]);
    }

    printf("\nModified files:\n");
    if (modifiedCount == 0) {
        printf("\tNo modified files.\n");
    } else {
        for (size_t i = 0; i < modifiedCount; ++i) printf("\t%s\n", modifiedFiles[i]);
    }

    printf("\nUntracked files:\n");
    if (untrackedCount == 0) {
        printf("\tNo untracked files.\n");
    } else {
        for (size_t i = 0; i < untrackedCount; ++i) printf("\t%s\n", untrackedFiles[i]);
    }
    printf("\n");

    /* Free everything */
    for (size_t i = 0; i < stagedCount; ++i) free(stagedFiles[i]);
    for (size_t i = 0; i < modifiedCount; ++i) free(modifiedFiles[i]);
    for (size_t i = 0; i < untrackedCount; ++i) free(untrackedFiles[i]);

    free(stagedFiles);
    free(modifiedFiles);
    free(untrackedFiles);

    freeTable(commitedTable);
    freeTable(stagedTable);

    if (outContent) free(outContent);

    fclose(indexFile);
    return 0;
}
