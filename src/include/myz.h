#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>

typedef enum {
  MFILE,
  MDIR,
  MROOT,
  MUNKNOWN
} FileType;
  
typedef struct {
  size_t myzNodeList;
  int myzNodeCount;
} Header;


typedef struct {
  int listIndex;
  char name[20];
} ArrayNode;

typedef struct {
  int arraySize;
  ArrayNode* data;
} NestedArray;


typedef struct {
  int uid;
  int gid;
  int permissions;
  char timestamp[20];
  char name[20];
  size_t fileLocation;
  size_t fileSize;
  FileType type;
  bool compressed;
  NestedArray array;
} MyzNode;


int myzInit(const char* myzFilename);

int myzInsert(char* inputFiles[], int count, bool compress);

MyzNode* readMyzList(int fd, int count);

int writeMyzList(int fd, int count, MyzNode *myzList);

MyzNode* insertEntry(int myzfd, char *filepath, char *filename, int rootDir, MyzNode* myzList, Header* head);
