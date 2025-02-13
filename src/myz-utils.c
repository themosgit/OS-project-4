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
  struct stat filestat;
  lstat(filepath, &filestat);
  if(findEntry(myzList, head->myzNodeCount, filestat.st_ino) != 0) return myzList;
  
  MyzNode* temp = (MyzNode*)realloc(myzList, sizeof(MyzNode) * ++head->myzNodeCount);
  if (temp == NULL) {
    perror("Error increasing myzList size");
  }
  myzList = temp;
  
  int nodeNum = head->myzNodeCount - 1;  
  myzList[nodeNum].uid = filestat.st_uid;
  myzList[nodeNum].gid = filestat.st_gid;
  myzList[nodeNum].permissions = filestat.st_mode & 0777;
  myzList[nodeNum].inode = filestat.st_ino;
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
    if (rootDir == 0) {
      strcpy(myzList[nodeNum].name, basename(filename));
      strcpy(myzList[rootDir].array.data[myzList[rootDir].array.arraySize - 1].name, basename(filename));
    }
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
    char fullpath[MAXPATH + 1];

    dir = opendir(filepath);
    if (dir == NULL) {
      perror("Error opendir");
      return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
      snprintf(fullpath, MAXPATH + 1, "%s/%s", filepath, entry->d_name);
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      myzList = insertEntry(myzfd, fullpath, entry->d_name, nodeNum, myzList, head);
    }
    closedir(dir);
  }

  return myzList;
}


int extractEntry(int myzfd, MyzNode* myzList, int index, char* path) {
  if (myzList[index].type == MFILE) {
    int outfd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (outfd == -1) {
      perror("Error creating file");
      return 1;
    }

    ssize_t bytes_read;
    size_t bytes_to_read = myzList[index].fileSize;
    char buffer[BUFFER_SIZE];

    lseek(myzfd, myzList[index].fileLocation, SEEK_SET);
    while (bytes_to_read > BUFFER_SIZE) {
      bytes_read = read(myzfd, buffer, BUFFER_SIZE);
      if (bytes_read != BUFFER_SIZE) {
        perror("Error reading from archive file");
        return 0;
      }
      write(outfd, buffer, BUFFER_SIZE);
      bytes_to_read -= BUFFER_SIZE;
    }

    bytes_read = read(myzfd, buffer, bytes_to_read);
    if (bytes_read != bytes_to_read) {
      perror("Error reading archive file");
    }
    write(outfd, buffer, bytes_to_read);
    close(outfd);

  } else if (myzList[index].type == MDIR || myzList[index].type == MROOT) {
    if (mkdir(path, 0777) != 0) {
      printf("path : %s\n", path);
      perror("Error creating directory");
      return 1;
    }

    char fullpath[MAXPATH + 1];
    for (int i = 0; i < myzList[index].array.arraySize; ++i) {
      if ((strcmp(myzList[index].array.data[i].name, ".") == 0) ||
           strcmp(myzList[index].array.data[i].name, "..") == 0) continue;

      snprintf(fullpath, MAXPATH + 1, "%s/%s", path, myzList[index].array.data[i].name);
      extractEntry(myzfd, myzList, myzList[index].array.data[i].listIndex, fullpath);
    }
  }
  return 0;
}





int findEntry(MyzNode* myzList, int myzNodeCount, int EntryInode) {
  int i = 1;
  while (myzList[i].inode != EntryInode && i < myzNodeCount) ++i;
  if (i == myzNodeCount) {
    return 0;
  } else {
    return i;
  }
}




int printTree(MyzNode* myzList, int rootDir, int level) {
  int index;
  for (int i = 0; i < myzList[rootDir].array.arraySize; ++i) {
    if ((strcmp(myzList[rootDir].array.data[i].name, ".") == 0) ||
         strcmp(myzList[rootDir].array.data[i].name, "..") == 0) continue;
     
    index = myzList[rootDir].array.data[i].listIndex;
    if(myzList[index].type == MFILE){
      for (int j = 0; j < level; ++j) printf("   ");
      printf("|->%s\n",myzList[index].name);

    } else if (myzList[index].type == MDIR){
      for (int j = 0; j < level; ++j) printf("   ");
      printf("|->%s/\n", myzList[index].name);
      printTree(myzList, index, level + 1);
    }
  }
  return 0;
}
