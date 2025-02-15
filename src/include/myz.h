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
#include <libgen.h>
#include <sys/wait.h>

typedef enum {
  MFILE,
  MDIR,
  MLINK,
  MROOT,
  MDELETED
} FileType;
  
typedef struct {
  size_t myzNodeList;
  int myzNodeCount;
} Header;


typedef struct {
  int listIndex;
  char name[32];
} ArrayNode;

typedef struct {
  int arraySize;
  ArrayNode* data;
} NestedArray;


typedef struct {
  int uid;
  int gid;
  int permissions;
  int inode;
  char timestamp[20];
  char name[32];
  size_t fileLocation;
  size_t fileSize;
  FileType type;
  bool compressed;
  NestedArray array;
} MyzNode;



int myzInit(const char* myzFilename);

int myzInsert(char* inputFiles[], int count, bool compress);

int myzExtract(char* inputFiles[], int count);

int myzMetadata(char* myzFilename);

int myzQuery(char* inputFiles[], int count);

int myzPrint(char* myzFilename);




int openMyz(char* myzFilename, Header* head, bool write);

MyzNode* readMyzList(int myzfd, Header head);

int writeMyzList(int myzfd, Header head, MyzNode *myzList);

MyzNode* insertEntry(int myzfd, char *filepath, char *filename, int rootDir, MyzNode* myzList, Header* head, bool compress);

MyzNode* insertFile(int myzfd, char* filepath, int nodeNum, MyzNode* myzList, Header* head);

MyzNode* insertLink(int myzfd, char* filepath, int nodeNum, MyzNode* myzList, Header* head);

int extractFile(int myzfd, MyzNode* myzList, int index, char* path);

int extractLink(int myzfd, MyzNode* myzList, int index, char* path);

int extractDir(int myzfd, MyzNode* myzList, int index, char* path);

int findEntry(MyzNode* myzList, int myzNodeCount, int EntryInode);

int printTree(MyzNode* myzList, int rootDir, int level);

void freeMyzList(MyzNode* myzList, int myzNodeCount);

int compressFile(char* filepath);

int decompressFile(int myzfd, size_t bytes, int outfd);

