#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "init.h"

#define MAX_BRANCH_LEN     128
#define MAX_COMMIT_MSG_LEN 512
#define MAX_PATH_LEN       512

static int validate_len(const char *label, const char *s, size_t max) {
    if (!s) { printf("%s is missing\n", label); return 0; }
    size_t n = strlen(s);
    if (n == 0) { printf("%s cannot be empty\n", label); return 0; }
    if (n > max) {
        printf("%s too long (> %zu chars)\n", label, max);
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: mockgit <command>\n");
        return 1;
    }

    const char *cmd = argv[1];

    // init
    if (strcmp(cmd, "init") == 0) {
        if (argc != 2) { printf("Usage: mockgit init\n"); return 1; }
        return makeInitFiles();
    }

    // status
    if (strcmp(cmd, "status") == 0) {
        if (argc != 2) { printf("Usage: mockgit status\n"); return 1; }
        return status();
    }

    // log
    if (strcmp(cmd, "log") == 0) {
        if (argc != 2) { printf("Usage: mockgit log\n"); return 1; }
        return logCommits();
    }

    // checkout <branch|commit>
    if (strcmp(cmd, "checkout") == 0) {
        if (argc != 3) { printf("Usage: mockgit checkout <branch|commit>\n"); return 1; }
        if (!validate_len("branch or commit", argv[2], MAX_BRANCH_LEN)) return 1;
        return checkout(argv[2]);
    }

    // branch <name>
    if (strcmp(cmd, "branch") == 0) {
        if (argc != 3) { printf("Usage: mockgit branch <name>\n"); return 1; }
        if (!validate_len("branch name", argv[2], MAX_BRANCH_LEN)) return 1;
        return branch(argv[2]);
    }

    // add <file...>
    if (strcmp(cmd, "add") == 0) {
        if (argc < 3) { printf("Usage: mockgit add <file...>\n"); return 1; }
        for (int i = 2; i < argc; ++i) {
            if (!validate_len("file path", argv[i], MAX_PATH_LEN)) return 1;
        }
        return addFiles(argc - 2, &argv[2]);
    }

    // commit -m "<message>"
    if (strcmp(cmd, "commit") == 0) {
        if (argc != 4) { printf("Usage: mockgit commit -m \"<message>\"\n"); return 1; }
        if (strcmp(argv[2], "-m") != 0) { printf("Usage: mockgit commit -m \"<message>\"\n"); return 1; }
        if (!validate_len("commit message", argv[3], MAX_COMMIT_MSG_LEN)) return 1;
        return commit(argv[2], argv[3]);
    }

    printf("Unknown command: %s\n", cmd);
    return 1;
}
