#include "../src/include/myz.h"

int main(int argc, char *argv[])  {
  if (argc != 2) {
    printf("Usage: %s <myz-archive-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int fd = open(argv[1], O_RDONLY);

  Header testHeader;
  MyzNode testMyzNode;

  read(fd, &testHeader, sizeof(Header));

  printf("Obtained header\n");
  printf("Size of myz file: %ld\n", testHeader.myzSize);
  printf("Number of myz nodes in file: %d\n", testHeader.myzNodeCount);
  printf("Location of myz node list: %ld\n", testHeader.myzNodeList);
  printf("Size of myz node chunks: %ld\n", testHeader.myzChunkSize);

  read(fd, &testMyzNode, sizeof(MyzNode));

  printf("Obtained myz node\n");
  printf("Type:");
  if (testMyzNode.type == MFILE) {
    printf("Regular file\n"); 
  } else if (testMyzNode.type == MDIR) {
    printf("Directory\n");
  } else if (testMyzNode.type == MROOT) {
    printf("Root node\n");
  } else {
    printf("Unknow\n");
  }
  printf("Name of file: %s\n", testMyzNode.name);
  printf("Timestamp: %s\n", testMyzNode.timestamp);
  
  read(fd, &testMyzNode.array.arraySize, sizeof(int));
  
  printf("Obtained array size\n");
  printf("Array size of node is: %d\n", testMyzNode.array.arraySize);

  testMyzNode.array.data = (ArrayNode*)malloc(sizeof(ArrayNode) * testMyzNode.array.arraySize);
  for (int i = 0; i < testMyzNode.array.arraySize; ++i) {
    read(fd, &testMyzNode.array.data[i], sizeof(ArrayNode));
    printf("Array Node: %d. Entry: %s in myz node: %d\n", i, testMyzNode.array.data[i].name, testMyzNode.array.data[i].listIndex);
  }

  free(testMyzNode.array.data);

  close(fd);
  
  
  return EXIT_SUCCESS;
}
