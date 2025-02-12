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
  head.myzChunkSize = sizeof(MyzNode) + sizeof(int) + root.array.arraySize * sizeof(ArrayNode);
  head.myzSize = sizeof(head) + head.myzChunkSize * head.myzNodeCount;


  int fd = open(myzFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

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

  free(root.array.data);
  return 0;
}

int writeMyzList(int fd, int count, MyzNode *myzNodeList) {
  for (int i = 0; i < count; ++i) {
    if (write(fd, &myzNodeList[i], sizeof(MyzNode)) == -1) {
      perror("Error writing MyzNode");
      close(fd);
      return 1;
    }
    if (write(fd, &myzNodeList[i].array.arraySize, sizeof(int)) == -1) {
      perror("Error writing MyzNode");
      close(fd);
      return 1;
    }
    for (int j = 0; j < myzNodeList[i].array.arraySize; ++j) {
      if (write(fd, &myzNodeList[i].array.data[j], sizeof(ArrayNode)) == -1) {
        perror("Error writing MyzNode array");
        close(fd);
        return 1;
      }
    }
  }
  return 0;
}
