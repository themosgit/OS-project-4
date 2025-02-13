#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "include/myz.h"

#define BUFFER_SIZE 4096
#define MAXPATH 256


int myzInit(const char* myzFilename) {
  MyzNode root;

  root.uid = 1;
  root.gid = 1;
  root.permissions = 1;

  time_t timenow;
  time(&timenow);
  strftime(root.timestamp, sizeof(root.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&timenow));

  strcpy(root.name, myzFilename);

  root.fileLocation = 0;
  root.fileSize = 0;
  root.type = MROOT;
  root.compressed = false;

  root.array.arraySize = 2;
  root.array.data = (ArrayNode*)malloc(sizeof(ArrayNode) * root.array.arraySize);
  
  root.array.data[0].listIndex = 0;
  strcpy(root.array.data[0].name, ".");
  root.array.data[1].listIndex = 0;
  strcpy(root.array.data[1].name, "..");

  Header head;
  head.myzNodeList = sizeof(head);
  head.myzNodeCount = 1; 


  int fd = open(myzFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd == -1) {
    perror("Error in myzInit open");
    return 1;
  }

  if (write(fd, &head, sizeof(Header)) == -1) {
    perror("Error in writing myz header to file\n");
    close(fd);
    return 1;
  }

  if (writeMyzList(fd, head.myzNodeCount, &root) == 1) {
    perror("Error in writeMyzList");
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}


int myzInsert(char* inputFiles[], int count, bool compress) {
  int myzfd = open(inputFiles[0], O_RDWR);
  if (myzfd == -1) {
    perror("Error opening archive file (myzInsert)");
    return 1;
  }

  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  if (bytes_read != sizeof(Header)) {
    perror("Incorrectly read header structure");
    return 1;
  }
  printf("Myz node list %ld\n", head.myzNodeList);
  printf("Myz node count %d\n", head.myzNodeCount);
  
  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);
  //TO DO: check if file is already in the archive
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    printf("%s\n", inputFiles[i]);
    myzList = insertEntry(myzfd, inputFiles[i], inputFiles[i], 0, myzList, &head);
  }
  lseek(myzfd, 0, SEEK_SET);
  write(myzfd, &head, sizeof(Header));
  lseek(myzfd, head.myzNodeList, SEEK_SET);

  if (writeMyzList(myzfd, head.myzNodeCount, myzList) == 1) {
    perror("Error in writeMyzList");
  };

  free(myzList);
  close(myzfd);
  return 0;
}


//TO DO: improve error handling
MyzNode* readMyzList(int fd, int count) {
  MyzNode* myzList = (MyzNode*)malloc(sizeof(MyzNode) * count);
  if (myzList == NULL) {
    perror("Error allocating memory for myzList");
    close(fd);
    return NULL;
  }

  ssize_t bytes_read;
  for (int i = 0; i < count; ++i) {
    bytes_read = read(fd, &myzList[i], sizeof(MyzNode));
    if (bytes_read != sizeof(MyzNode)) {
      perror("Error reading myzNode");
      free(myzList);
      close(fd);
      return NULL;
    }
    read(fd, &myzList[i].array.arraySize, sizeof(int));
    if (myzList[i].array.arraySize == 0) continue;

    myzList[i].array.data = (ArrayNode*)malloc(sizeof(ArrayNode) * myzList[i].array.arraySize);
    for (int j = 0; j < myzList[i].array.arraySize; ++j) {
      read(fd, &myzList[i].array.data[j], sizeof(ArrayNode));
    }
  }
  return myzList;
}


int writeMyzList(int fd, int count, MyzNode *myzList) {
  for (int i = 0; i < count; ++i) {
    if (write(fd, &myzList[i], sizeof(MyzNode)) == -1) {
      perror("Error writing MyzNode");
      close(fd);
      return 1;
    }
    if (write(fd, &myzList[i].array.arraySize, sizeof(int)) == -1) {
      perror("Error writing MyzNode");
      close(fd);
      return 1;
    }
    if (myzList[i].array.arraySize == 0) continue;
    for (int j = 0; j < myzList[i].array.arraySize; ++j) {
      if (write(fd, &myzList[i].array.data[j], sizeof(ArrayNode)) == -1) {
        perror("Error writing MyzNode array");
        close(fd);
        return 1;
      }
    }
    free(myzList[i].array.data);
  }
  return 0;
}

MyzNode* insertEntry(int myzfd, char *filepath, char *filename, int rootDir, MyzNode* myzList, Header* head) {
  MyzNode* temp = (MyzNode*)realloc(myzList, sizeof(MyzNode) * ++head->myzNodeCount);
  if (temp == NULL) {
    perror("Error increasing myzList size");
  }
  myzList = temp;

  struct stat filestat;
  lstat(filepath, &filestat);

  int nodeNum = head->myzNodeCount - 1;  
  myzList[nodeNum].uid = filestat.st_uid;
  myzList[nodeNum].gid = filestat.st_gid;
  myzList[nodeNum].permissions = filestat.st_mode & 0777;
  myzList[nodeNum].fileSize = filestat.st_size;
  myzList[nodeNum].compressed = false;

  strcpy(myzList[nodeNum].name, filename);
  strftime(myzList[nodeNum].timestamp, sizeof(myzList[nodeNum].timestamp), "%Y-%m-%d %H:%M:%S", localtime(&filestat.st_atime));

  ArrayNode* tempData = (ArrayNode*)realloc(myzList[rootDir].array.data, sizeof(ArrayNode) * ++myzList[rootDir].array.arraySize);
  if (tempData == NULL) {
    perror("Error increasing nested array size");
  }
  myzList[rootDir].array.data = tempData;
  myzList[rootDir].array.data[myzList[rootDir].array.arraySize - 1].listIndex = nodeNum;
  strcpy(myzList[rootDir].array.data[myzList[rootDir].array.arraySize - 1].name, filename);

  if (S_ISREG(filestat.st_mode)) {
    myzList[nodeNum].type = MFILE;
    myzList[nodeNum].array.arraySize = 0;
    myzList[nodeNum].fileLocation = head->myzNodeList;
    head->myzNodeList += filestat.st_size;

    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];
    int inputfd = open(filepath, O_RDONLY);
    lseek(myzfd, myzList[nodeNum].fileLocation, SEEK_SET);
    while ((bytes_read = read(inputfd, buffer, BUFFER_SIZE)) > 0) {
      write(myzfd, buffer, BUFFER_SIZE);
    }
    close(inputfd);
    lseek(myzfd, head->myzNodeList, SEEK_SET);
    printf("Success\n");

  } else if (S_ISDIR(filestat.st_mode)) {
    myzList[nodeNum].type = MDIR;
    myzList[nodeNum].fileLocation = 0;

    myzList[nodeNum].array.arraySize = 2;
    myzList[nodeNum].array.data = (ArrayNode*)malloc(sizeof(ArrayNode) * 2);

    myzList[nodeNum].array.data[0].listIndex = nodeNum;
    strcpy(myzList[nodeNum].array.data[0].name, ".");
    myzList[nodeNum].array.data[1].listIndex = rootDir;
    strcpy(myzList[nodeNum].array.data[1].name, "..");

    DIR *dir;
    struct dirent *entry;
    char fullpath[MAXPATH];

    dir = opendir(filepath);
    if (dir == NULL) {
      perror("Error opendir");
      return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
      snprintf(fullpath, MAXPATH, "%s/%s", filepath, entry->d_name);
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      myzList = insertEntry(myzfd, fullpath, entry->d_name, nodeNum, myzList, head);
    }
    closedir(dir);
  }
  return myzList;
}
