#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <directory>\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  char directory[1024];

  getcwd(directory, sizeof(directory));
  printf("Current working directory: %s\n", directory);
  int fd = open(argv[1], O_RDONLY | O_DIRECTORY);
  getcwd(directory, sizeof(directory));
  printf("Current working directory after using open: %s\n", directory);
  fchdir(fd);
  getcwd(directory, sizeof(directory));
  printf("Current working directory after using fchdir: %s\n", directory);
  chdir("..");
  getcwd(directory, sizeof(directory));
  printf("Current working directory after returning with chdir(..): %s\n", directory);
  close(fd);
  return EXIT_SUCCESS;

}
