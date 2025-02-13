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
    perror("Error opening archive file");
    return 1;
  }

  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  if (bytes_read != sizeof(Header)) {
    perror("Error read header structure");
    return 1;
  }
  
  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);

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




int myzExtract(char* inputFiles[], int count) {
  int myzfd = open(inputFiles[0], O_RDONLY);
  if (myzfd == -1) {
    perror("Error opening archive file");
    return 1;
  }
  printf("extractEntry\n");
  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  printf("extractEntry\n");
  if (bytes_read != sizeof(Header)) {
    perror("Error reading structure");
    return 1;
  }
  
  
  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);
  
  int index;
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    
    lstat(inputFiles[i], &filestat);
    index = findEntry(myzList, head.myzNodeCount, filestat.st_ino);
    extractEntry(myzfd, myzList, index, myzList[index].name);
  }

  if (count == 1) {
    printf("extracting all\n");
    extractEntry(myzfd, myzList, 0, myzList[0].name);
  }

  close(myzfd);
  for (int i = 0; i < head.myzNodeCount; ++i) {
    if (myzList[i].array.arraySize == 0) continue;
    free(myzList[i].array.data);
  }
  free(myzList);
  return 0;
}




int myzMetadata(char* myzFilename) {
  int myzfd = open(myzFilename, O_RDONLY);
  if (myzfd == -1) {
    perror("Error opening archive file");
    return 1;
  }

  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  if(bytes_read != sizeof(Header)) {
    perror("Error reading header structure");
    return 1;
  }

  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);
  close(myzfd);

  for (int i = 1; i < head.myzNodeCount; ++i) {
    printf("Entry name: %s", myzList[i].name);
    if (myzList[i].type == MFILE) {
      printf("  Regular file\n");
    } else if (myzList[i].type == MDIR) {
      printf("  Directory\n");
    }
    printf("   Owner id: %d. Group id: %d.\n", myzList[i].uid, myzList[i].gid);
    printf("   Access rights: %o. File size: %ld bytes.\n", myzList[i].permissions, myzList[i].fileSize);
    printf("   Timestamp: %s.\n\n", myzList[i].timestamp);
  }
  
  for (int i = 0; i < head.myzNodeCount; ++i) {
    if (myzList[i].array.arraySize == 0) continue;
    free(myzList[i].array.data);
  }
  free(myzList);
  return 0;
}




int myzQuery(char* inputFiles[], int count) {
  int myzfd = open(inputFiles[0], O_RDONLY);
  if (myzfd == -1) {
    perror("Error opening archive file");
    return 1;
  }

  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  if(bytes_read != sizeof(Header)) {
    perror("Error reading header structure");
    return 1;
  }

  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);
  close(myzfd);
  
  struct stat filestat;
  for (int i = 1; i < count; ++i) {
    lstat(inputFiles[i], &filestat);
    if (findEntry(myzList, head.myzNodeCount, filestat.st_ino) != 0) {
      printf("Found entry: %s in %s\n", inputFiles[i], inputFiles[0]);
    } else  {
      printf("%s not found in %s\n", inputFiles[i], inputFiles[0]);
    }
  }

  for (int i = 0; i < head.myzNodeCount; ++i) {
    if (myzList[i].array.arraySize == 0) continue;
    free(myzList[i].array.data);
  }
  free(myzList);
  return 0;
}




int myzPrint(char* myzFilename) {
  int myzfd = open(myzFilename, O_RDONLY);
  if (myzfd == -1) {
    perror("Error opening archive file");
    return 1;
  }

  Header head;
  ssize_t bytes_read = read(myzfd, &head, sizeof(Header));
  if(bytes_read != sizeof(Header)) {
    perror("Error reading header structure");
    return 1;
  }

  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = readMyzList(myzfd, head.myzNodeCount);
  close(myzfd);

  printTree(myzList, 0, 0);

  for (int i = 0; i < head.myzNodeCount; ++i) {
    if (myzList[i].array.arraySize == 0) continue;
    free(myzList[i].array.data);
  }
  free(myzList);
  return 0;
}
