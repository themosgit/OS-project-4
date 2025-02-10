#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

/* C program to test functionality of readlink sys call */ 

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <symbolic_link>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *link_path = argv[1];
  char target_path[PATH_MAX];
  ssize_t len = readlink(link_path, target_path, PATH_MAX - 1);

  if (len == -1 ) {
    perror("readlink");
    exit(EXIT_FAILURE);
  }

  target_path[len] = '\0';
  printf("Symbolic link: %s\n", link_path);
  printf("Target: %s\n", target_path);
  return EXIT_SUCCESS;
}
