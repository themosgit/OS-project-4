#include "../src/include/myz.h"

int main(int argc, char *argv[])  {
  if (argc != 2) {
    printf("Usage: %s <myz-archive-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int fd = open(argv[1], O_RDONLY);

  Header head;

  read(fd, &head, sizeof(Header));

  printf("Obtained header\n");
  printf("Number of myz nodes in file: %d\n", head.myzNodeCount);
  printf("Location of myz node list: %ld\n", head.myzNodeList);

  lseek(fd, head.myzNodeList, SEEK_SET);

  MyzNode* myzList = readMyzList(fd, head.myzNodeCount);

  for (int j = 0; j < head.myzNodeCount; ++j) {
    printf("\n\nObtained myz node\n");
    printf("Type:");
    if (myzList[j].type == MFILE) {
      printf("Regular file\n"); 
    } else if (myzList[j].type == MDIR) {
      printf("Directory\n");
    } else if (myzList[j].type == MROOT) {
      printf("Root node\n");
    } else {
      printf("Unknow\n");
    }
    printf("Name of file: %s\n", myzList[j].name);
    printf("Timestamp: %s\n", myzList[j].timestamp);
  
    if (myzList[j].type == MFILE) continue;

    printf("Obtained array size\n");
    printf("Array size of node is: %d\n", myzList[j].array.arraySize);

    for (int i = 0; i < myzList[j].array.arraySize; ++i) {
      printf("Array Node: %d. Entry: %s in myz node: %d\n", i, myzList[j].array.data[i].name, myzList[j].array.data[i].listIndex);
    }

    free(myzList[j].array.data);
  }
  free(myzList);
  close(fd);
  
  
  return EXIT_SUCCESS;
}
