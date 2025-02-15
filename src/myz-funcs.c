#include "include/myz.h"


int myzInit(const char* myzFilename) {
  MyzNode root;

  root.uid = 1;
  root.gid = 1;
  root.permissions = 1;
  root.inode = 0;

  time_t timenow;
  time(&timenow);
  strftime(root.timestamp, sizeof(root.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&timenow));

  snprintf(root.name, sizeof(root.name), "%s-root", myzFilename);

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

  if (writeMyzList(fd, head, &root) == 1) {
    perror("Error in writeMyzList");
    close(fd);
    return 1;
  }
close(fd);
  return 0;
}



int myzInsert(char* inputFiles[], int count, bool compress) {
  Header head;
  bool readWrite = true;
  int myzfd = openMyz(inputFiles[0], &head, readWrite);
  MyzNode* myzList = readMyzList(myzfd, head);

  for (int i = 1; i < count; ++i) {
    myzList = insertEntry(myzfd, inputFiles[i], inputFiles[i], 0, myzList, &head, compress);
  }

  if (writeMyzList(myzfd, head, myzList) == 1) {
    perror("Error in writeMyzList");
  };
  freeMyzList(myzList, head.myzNodeCount);
  close(myzfd);
  return 0;
}



int myzExtract(char* inputFiles[], int count) {
  Header head;
  bool readWrite = false;
  int myzfd = openMyz(inputFiles[0], &head, readWrite);
  MyzNode* myzList = readMyzList(myzfd, head);

  int index;
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    lstat(inputFiles[i], &filestat);
    index = findEntry(myzList, head.myzNodeCount, filestat.st_ino);
    if (myzList[index].type == MFILE) {
       myzextractFile(myzfd, myzList, index, myzList[index].name);
    } else if (myzList[index].type == MDIR) {
       extractDir(myzfd, myzList, index, myzList[index].name);
    } 
  }

  if (count == 1) {
    extractDir(myzfd, myzList, 0, myzList[0].name);
  }
  freeMyzList(myzList, head.myzNodeCount);
  close(myzfd);
  return 0;
}



int myzDelete(char* inputFiles[], int count) {
  Header head;
  bool readWrite = true;
  int myzfd = openMyz(inputFiles[0], &head, readWrite);
  MyzNode* myzList = readMyzList(myzfd, head);

  int index;
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    lstat(inputFiles[0], &filestat);
    index = findEntry(myzList, head.myzNodeCount, filstat.st_ino);
    if (index == 0) continue;
    
  
  }
  printf("ets\n");
}



int myzMetadata(char* myzFilename) {
  Header head;
  bool readWrite = false;
  int myzfd = openMyz(myzFilename, &head, readWrite);
  MyzNode* myzList = readMyzList(myzfd, head);
  close(myzfd);

  for (int i = 1; i < head.myzNodeCount; ++i) {
    printf("Entry name: %s", myzList[i].name);
    if (myzList[i].type == MFILE) {
      printf("  Regular file\n");
    } else if (myzList[i].type == MLINK) {
      printf("  Symbolic link\n");
    } else if (myzList[i].type == MDIR) {
      printf("  Directory\n");
    }
    printf("   Owner id: %d. Group id: %d.\n", myzList[i].uid, myzList[i].gid);
    printf("   Access rights: %o. File size: %ld bytes.\n", myzList[i].permissions, myzList[i].fileSize);
    printf("   Timestamp: %s.\n\n", myzList[i].timestamp);
  }
  freeMyzList(myzList, head.myzNodeCount);
  return 0;
}



int myzQuery(char* inputFiles[], int count) {
  Header head;
  bool readWrite = false;
  int myzfd = openMyz(inputFiles[0], &head, readWrite);
  MyzNode* myzList = readMyzList(myzfd, head);
  close(myzfd);

  int index;
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    lstat(inputFiles[i], &filestat);
    if ((index = findEntry(myzList, head.myzNodeCount, filestat.st_ino)) != 0) {
      printf("Found entry: %s in %s\n", myzList[index].name, inputFiles[0]);
    } else  {
      printf("%s not found in %s\n", myzList[i].name, inputFiles[0]);
    }
  }
  freeMyzList(myzList, head.myzNodeCount);
  return 0;
}



int myzPrint(char* myzFilename) {
  Header head;
  bool write = false;
  int myzfd = openMyz(myzFilename, &head, write);
  MyzNode* myzList = readMyzList(myzfd, head);
  close(myzfd);

  printTree(myzList, 0, 0);
  freeMyzList(myzList, head.myzNodeCount);
  return 0;
}
