#include "include/myz.h"

#define BUFFER_SIZE 4096
#define MAXPATH 256

//opens archive files fd and reades header structure.
int openMyz(char* myzFilename, Header* head, bool write) {
  int myzfd;
  if (write)  myzfd = open(myzFilename, O_RDWR);
  else myzfd = open(myzFilename, O_RDONLY);

  if (myzfd == -1) {
    perror("Error opening archive file");
    return -1;
  }
  
  ssize_t bytes_read = read(myzfd, head, sizeof(Header));
  if (bytes_read != sizeof(Header)) {
    perror("Error read header structure");
    return -1;
  }
  return myzfd;
}


//Retrieves the myzNodeList of an archive file
MyzNode* readMyzList(int myzfd, Header head) {
  lseek(myzfd, head.myzNodeList, SEEK_SET);
  MyzNode* myzList = (MyzNode*)malloc(sizeof(MyzNode) * head.myzNodeCount);
  if (myzList == NULL) {
    perror("Error allocating memory for myzList");
    close(myzfd);
    return NULL;
  }
  ssize_t bytes_read;
  for (int i = 0; i < head.myzNodeCount; ++i) {
    bytes_read = read(myzfd, &myzList[i], sizeof(MyzNode));
    if (bytes_read != sizeof(MyzNode)) {
      perror("Error reading myzNode");
      free(myzList);
      close(myzfd);
      return NULL;
    }
    read(myzfd, &myzList[i].array.arraySize, sizeof(int));
    if (myzList[i].array.arraySize == 0) continue;

    //if the node represents a directory the nested array is allocated and read
    myzList[i].array.data = (ArrayNode*)malloc(sizeof(ArrayNode) * myzList[i].array.arraySize);
    if (myzList[i].array.data == NULL) {
      perror("Error allocating memory for myzList");
      close(myzfd);
      return NULL;
    }

    for (int j = 0; j < myzList[i].array.arraySize; ++j) {
      read(myzfd, &myzList[i].array.data[j], sizeof(ArrayNode));
    }
  }
  return myzList;
}


//Writes the myzNodeList to the archive file
int writeMyzList(int myzfd, Header head, MyzNode *myzList) {
  lseek(myzfd, 0, SEEK_SET);
  write(myzfd, &head, sizeof(Header));
  lseek(myzfd, head.myzNodeList, SEEK_SET);
  //after the fd is set to the correct place within the archive file
  //all myzNodes and nested arrays are written
  for (int i = 0; i < head.myzNodeCount; ++i) {
    if (write(myzfd, &myzList[i], sizeof(MyzNode)) == -1) {
      perror("Error writing MyzNode");
      close(myzfd);
      return 1;
    }
    if (write(myzfd, &myzList[i].array.arraySize, sizeof(int)) == -1) {
      perror("Error writing MyzNode");
      close(myzfd);
      return 1;
    }
    if (myzList[i].array.arraySize == 0) continue;
    for (int j = 0; j < myzList[i].array.arraySize; ++j) {
      if (write(myzfd, &myzList[i].array.data[j], sizeof(ArrayNode)) == -1) {
        perror("Error writing MyzNode array");
        close(myzfd);
        return 1;
      }
    }
  }
  return 0;
}


/*Inserts an entry in the archive file recursive
 * calls are made if the entry is a directory. */
MyzNode* insertEntry(int myzfd, char *filepath, char *filename, int rootDir, MyzNode* myzList, Header* head, bool compress) {
  struct stat filestat; 
  lstat(filepath, &filestat);
  //check if the file exists in the archive
  if(findEntry(myzList, head->myzNodeCount, filestat.st_ino) != 0) return myzList;
  
  //reallocate myzNodeList to accomadate new node 
  MyzNode* temp = (MyzNode*)realloc(myzList, sizeof(MyzNode) * ++head->myzNodeCount);
  if (temp == NULL) {
    perror("Error increasing myzList size");
  }
  myzList = temp;

  //initialize new entry 
  int nodeNum = head->myzNodeCount - 1;  
  myzList[nodeNum].uid = filestat.st_uid;
  myzList[nodeNum].gid = filestat.st_gid;
  myzList[nodeNum].permissions = filestat.st_mode & 0777;
  myzList[nodeNum].inode = filestat.st_ino;
  myzList[nodeNum].fileSize = filestat.st_size;
  myzList[nodeNum].compressed = compress;

  //extract filename from path if this is the root node
  if (rootDir == 0) filename = basename(filename);
  strcpy(myzList[nodeNum].name, filename);
  strftime(myzList[nodeNum].timestamp, sizeof(myzList[nodeNum].timestamp), "%Y-%m-%d %H:%M:%S", localtime(&filestat.st_atime));

  //reallocate nested array of node to accomadate new entry
  ArrayNode* tempData = (ArrayNode*)realloc(myzList[rootDir].array.data, sizeof(ArrayNode) * ++myzList[rootDir].array.arraySize);
  if (tempData == NULL) {
    perror("Error increasing nested array size");
  }
  //initialize nested array entry
  myzList[rootDir].array.data = tempData;
  myzList[rootDir].array.data[myzList[rootDir].array.arraySize - 1].listIndex = nodeNum;
  strcpy(myzList[rootDir].array.data[myzList[rootDir].array.arraySize - 1].name, filename);


  if (S_ISREG(filestat.st_mode)) {

    myzList = insertFile(myzfd, filepath, nodeNum, myzList, head);

  } else if (S_ISLNK(filestat.st_mode)) {

    myzList = insertLink(myzfd, filepath, nodeNum, myzList, head);

  } else if (S_ISDIR(filestat.st_mode)) {
    
    //initialize directory node
    myzList[nodeNum].type = MDIR;
    myzList[nodeNum].fileLocation = 0;
    myzList[nodeNum].array.arraySize = 2;

    //initialize directory node's nested array
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
    //Recursively call insertEntry for directory contents
    while ((entry = readdir(dir)) != NULL) {
      snprintf(fullpath, MAXPATH + 1, "%s/%s", filepath, entry->d_name);
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      myzList = insertEntry(myzfd, fullpath, entry->d_name, nodeNum, myzList, head, compress);
    }
    closedir(dir);
  } 
  return myzList;
}


//handles file insertion and calls compressFile func if compression is requested
MyzNode* insertFile(int myzfd, char* filepath, int nodeNum, MyzNode* myzList, Header* head) {
  //initialization of file specific myzNode values
  myzList[nodeNum].type = MFILE;
  myzList[nodeNum].array.arraySize = 0;
  myzList[nodeNum].fileLocation = head->myzNodeList;
  //handles file compression and correct filesize 
  if (myzList[nodeNum].compressed) {
    compressFile(filepath);
    snprintf(filepath, 8, "temp.gz");
    struct stat filestat;
    lstat("temp.gz", &filestat);
    myzList[nodeNum].fileSize = filestat.st_size;
  }
  head->myzNodeList += myzList[nodeNum].fileSize;


  int inputfd = open(filepath, O_RDONLY);
  //writing input file to archive
  ssize_t bytes_read;
  char buffer[BUFFER_SIZE];
  lseek(myzfd, myzList[nodeNum].fileLocation, SEEK_SET);
  while ((bytes_read = read(inputfd, buffer, BUFFER_SIZE)) > 0) {
    write(myzfd, buffer, BUFFER_SIZE);
  }
  //unlinkink temp file created be compress
  close(inputfd);
  if (myzList[nodeNum].compressed) {
    unlink(filepath);
  }
  return myzList;
}


//handles link insertion
MyzNode* insertLink(int myzfd, char* filepath, int nodeNum, MyzNode* myzList, Header* head) {
  //initialization of link specific myznode values
  myzList[nodeNum].type = MLINK;
  myzList[nodeNum].array.arraySize = 0;
  myzList[nodeNum].fileLocation = head->myzNodeList;

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = readlink(filepath, buffer, BUFFER_SIZE);
  buffer[bytes_read] = '\0';
  //writing link path to archive file
  head->myzNodeList += strlen(buffer);
  myzList[nodeNum].fileSize = strlen(buffer);
  lseek(myzfd, myzList[nodeNum].fileLocation, SEEK_SET);
  write(myzfd, buffer, strlen(buffer));
  return myzList;
 }


//Marks entries as deleted and modifies other myznode file fileLocation
//and archive file size, called Recursively for directories
MyzNode* deleteEntry(MyzNode*myzList, int index, Header* head) {
  if (myzList[index].type == MFILE || myzList[index].type == MLINK) {
    myzList[index].type = MDELETED;
    for (int i = index + 1; i < head->myzNodeCount; ++i) {
      if(myzList[i].type == MFILE || myzList[i].type == MLINK) {
        myzList[i].fileLocation -= myzList[index].fileSize;
      }
    }
    head->myzNodeList -= myzList[index].fileSize;
  } else if (myzList[index].type == MDIR) {
    myzList[index].type = MDELETED;
    for (int i = 0; i < myzList[index].array.arraySize; ++i) {
      if ((strcmp(myzList[index].array.data[i].name, ".") == 0) ||
           strcmp(myzList[index].array.data[i].name, "..") == 0) continue;
      myzList = deleteEntry(myzList, i, head);
    }
  }
  return myzList;
}



//rewrites files based on modifications to myzNodes after deleteEntry
//overwriting deleted entries in the archive. tempList is the updated
//myzNodeList while myzList holds the previous instance
MyzNode* refreshMyz(int myzfd, MyzNode* tempList, MyzNode* myzList, int myzNodeCount, int index) {
  size_t writeLocation, bytes_to_read; 
  ssize_t bytes_read;
  char buffer[BUFFER_SIZE];
  for (int i = index + 1; i < myzNodeCount; ++i) {
    writeLocation = myzList[i].fileLocation;
    if (writeLocation == 0) continue; 
    bytes_to_read = tempList[i].fileSize;
    lseek(myzfd, tempList[i].fileLocation, SEEK_SET);
    while (bytes_to_read > BUFFER_SIZE) {
      bytes_read = read(myzfd, buffer, BUFFER_SIZE);
      lseek(myzfd, writeLocation, SEEK_SET);
      write(myzfd, buffer, BUFFER_SIZE);
      writeLocation += BUFFER_SIZE;
      bytes_to_read -= BUFFER_SIZE;
    }
    bytes_to_read = read(myzfd, buffer, bytes_to_read);
    lseek(myzfd, writeLocation, SEEK_SET);
    write(myzfd, buffer, bytes_to_read); 
  }
  for (int i = 0; i < myzNodeCount; ++i) {
    if (myzList[i].type == MDELETED) {
      myzList[i].fileLocation = 0;
      myzList[i].fileSize = 0;
    }
  }
  return myzList;
}


//handles file extraction from archive file
int extractFile(int myzfd, MyzNode* myzList, int index, char* path) {
  int outfd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (outfd == -1) {
      perror("Error creating file");
      return 1;
  }
  ssize_t bytes_read;
  size_t bytes_to_read = myzList[index].fileSize;
  lseek(myzfd, myzList[index].fileLocation, SEEK_SET);
  //redirects extraction for compressed file 
  if (myzList[index].compressed) {
    return decompressFile(myzfd, bytes_to_read, outfd);
  }
  //bytes being read and written are dictated by stored filesize
  char buffer[BUFFER_SIZE];
  while (bytes_to_read > BUFFER_SIZE) {
    bytes_read = read(myzfd, buffer, BUFFER_SIZE);
    if (bytes_read != BUFFER_SIZE) {
      perror("Error reading from archive file");
      return 1;
    }
    write(outfd, buffer, BUFFER_SIZE);
    bytes_to_read -= BUFFER_SIZE;
  }
  bytes_to_read = read(myzfd, buffer, bytes_to_read);
  write(outfd, buffer, bytes_to_read);
  close(outfd);
  return 0;
}


//handles link extraction
int extractLink(int myzfd, MyzNode* myzList, int index, char* path) {
  char buffer[BUFFER_SIZE];
  lseek(myzfd, myzList[index].fileLocation, SEEK_SET);
  read(myzfd, buffer, myzList[index].fileSize);
  
  //getting path to create symlink in 
  //proper directory
  char dir[BUFFER_SIZE];
  getcwd(dir, sizeof(dir));
  char dirpath[strlen(path) - strlen(myzList[index].name)];
  snprintf(dirpath, sizeof(dirpath), "%s", path);
  chdir(dirpath);

  if (symlink(buffer, myzList[index].name) == -1) {
    perror("Error creating symlink");
    return 1;
  }
  //returning to previous working directory
  chdir(dir);
  return 0;
}


//handles directory extraction
int extractDir(int myzfd, MyzNode* myzList, int index, char* path) {
  if (mkdir(path, 0777) != 0) {
    perror("Error creating directory");
    return 1;
  }
  //loop for all directory entries ignoring . and ..
  char fullpath[MAXPATH + 1];
  for (int i = 0; i < myzList[index].array.arraySize; ++i) {
    if ((strcmp(myzList[index].array.data[i].name, ".") == 0) ||
         strcmp(myzList[index].array.data[i].name, "..") == 0) continue;

    //constructing path so that the next entry is created at the proper place
    snprintf(fullpath, MAXPATH + 1, "%s/%s", path, myzList[index].array.data[i].name);
    int nextNode = myzList[index].array.data[i].listIndex;
    
    if (myzList[nextNode].type == MFILE) {
      extractFile(myzfd, myzList, nextNode, fullpath);
    } else if (myzList[nextNode].type == MLINK) {
      extractLink(myzfd, myzList, nextNode, fullpath);
    } else if (myzList[nextNode].type == MDIR) {
      extractDir(myzfd, myzList, nextNode, fullpath);
    }
  }
  return 0;
}


//searches for an entry in the archive based on inode returns myzListNode index
int findEntry(MyzNode* myzList, int myzNodeCount, int EntryInode) {
  int i = 1;
  while (myzList[i].inode != EntryInode && i < myzNodeCount) ++i;
  if(myzList[i].type == MDELETED) return 0;
  if (i == myzNodeCount) return 0;
  return i;
}


//recursively prints all entries in archive file
int printTree(MyzNode* myzList, int rootDir, int level) {
  int index;
  for (int i = 0; i < myzList[rootDir].array.arraySize; ++i) {
    if ((strcmp(myzList[rootDir].array.data[i].name, ".") == 0) ||
         strcmp(myzList[rootDir].array.data[i].name, "..") == 0) continue;
     
    index = myzList[rootDir].array.data[i].listIndex;
    if(myzList[index].type == MFILE || myzList[index].type == MLINK){
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


//frees myzList and nested arrays
void freeMyzList(MyzNode* myzList, int myzNodeCount) {
  for (int i = 0; i < myzNodeCount; ++i) {
    if (myzList[i].array.arraySize == 0) continue;
    free(myzList[i].array.data);
  }
  free(myzList);
}


//handles compression of files
int compressFile(char* filepath) {
  int tempfd, infd;
  char tempname[8];
  //using a temproray file that will later be unlinked
  snprintf(tempname, 8, "temp.gz");
  tempfd = open(tempname, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (tempfd == -1) {
    perror("Error creating temp file");
    return 1;
  }

  pid_t pid;
  if((pid = fork()) == 0) {
    infd = open(filepath, O_RDONLY);
    //duplicates file descriptors for use in gzip
    if (dup2(infd, STDIN_FILENO) == -1) {
      perror("Error in dup2 infd");
      exit(1);
    }
    close(infd);

    if (dup2(tempfd, STDOUT_FILENO) == -1) {
      perror("Error in dup2 temp");
      exit(1);
    }
    close(tempfd);

    execlp("gzip", "gzip", "-c", NULL);
    perror("Error in execlp");
    exit(1);
  } 
  int status;
  waitpid(pid, &status, 0);
  close(tempfd);
  return 0;
}


//handles file decompression
int decompressFile(int myzfd, size_t bytes, int outfd) {
  //if the file is empty 
  if (bytes == 0) {
    close(outfd);
    return 0;
  }
  //temp file for moving compressed data 
  //from archive so it can be processed by gzip
  char tempname[8];
  snprintf(tempname, 8, "temp.gz");
  int tempfd = open(tempname, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (tempfd == -1) {
    perror("Error creating temp file");
    return 1;
  }
  //writing for archive to temp file
  ssize_t bytes_read;
  char buffer[BUFFER_SIZE];
  while (bytes > BUFFER_SIZE) {
    bytes_read = read(myzfd, buffer, BUFFER_SIZE);
    if (bytes_read != BUFFER_SIZE) {
      perror("Error reading from archive file");
      return 1;
    }

    write(tempfd, buffer, BUFFER_SIZE);
    bytes -= BUFFER_SIZE;
  } 
  bytes = read(myzfd, buffer, bytes);
  write(tempfd, buffer, bytes);
  lseek(tempfd, 0, SEEK_SET);
  pid_t pid;
  
  if ((pid = fork()) == 0) {
    //duplicating file descriptors for gzip
    if (dup2(tempfd, STDIN_FILENO) == -1) {
      perror("Error in dup2 decomp temp");
      exit(1);
    }
    close(tempfd);

    if (dup2(outfd, STDOUT_FILENO) == -1) {
      perror("Error in dup2 decomp outfd");
      exit(1);
    }
    close(outfd);

    execlp("gzip", "gzip", "-d", NULL);
    perror("Error in execlp");
    exit(1);
  }
  int status;
  waitpid(pid, &status, 0);
  close(outfd);
  close(tempfd);
  //unlinking temp file
  unlink(tempname);
  return 0;
}
