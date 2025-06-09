#ifndef INIT_H
#define INIT_H

int makeInitFiles();
int addFiles(int fileCount, char **fileList);
int commit(char *type, char *message);
int logCommits();
char *hashToBlob(FILE *file, unsigned char *buffer, unsigned char **outContent, long *outContentLen);


#endif