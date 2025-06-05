#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int logCommits() {
    FILE *head = fopen(".mockgit/HEAD", "r");
    if (!head) {
        perror("Error opening HEAD file");
        return 1;
    }

    char headHash[65];
    if (fscanf(head, "%64s", headHash) != 1) {
        perror("Error reading HEAD hash");
        fclose(head);
        return 1;
    }
    fclose(head);

    char *log = malloc(1);
    if (!log) {
        perror("Initial log malloc failed");
        return 1;
    }
    log[0] = '\0';
    size_t logSize = 1;

    char commitPath[128];
    snprintf(commitPath, sizeof(commitPath), ".mockgit/commits/%s", headHash);

    FILE *commitFile = fopen(commitPath, "r");
    while (commitFile != NULL) {
        char line[512];
        char parentHash[65] = "";
        int isParentLine = 1;

        logSize += strlen("=========================\n");
        log = realloc(log, logSize);
        strcat(log, "=========================\n");
        

        while (fgets(line, sizeof(line), commitFile)) {
            if (isParentLine) {
                // Handle parent hash line
                if (sscanf(line, "Parent: %64s", parentHash) != 1) {
                    strcpy(parentHash, "none");
                }
                isParentLine = 0;
            }

            size_t len = strlen(line);
            char *newLog = realloc(log, logSize + len + 1);
            if (!newLog) {
                perror("Realloc failed");
                free(log);
                fclose(commitFile);
                return 1;
            }
            log = newLog;
            strcat(log, line);
            logSize += len;
        }

        fclose(commitFile);

        if (strcmp(parentHash, "none") == 0) {
            break; // End of commit chain
        }
        
        snprintf(commitPath, sizeof(commitPath), ".mockgit/commits/%s", parentHash);
        commitFile = fopen(commitPath, "r");
    }

    printf("%s", log);
    free(log);
    return 0;
}
