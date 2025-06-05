#ifndef INIT_H
#define INIT_H

int makeInitFiles();
int addFiles(int fileCount, char **fileList);
int commit(char *type, char *message);
int logCommits();

#endif