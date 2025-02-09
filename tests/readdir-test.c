#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

/* C program to test functionality of readdir() sys call*/

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *dir_name = argv[1];
  DIR *dir = opendir(dir_name);

  if (dir == NULL) {
    perror("opendir");
    return EXIT_FAILURE;
  }

  struct dirent *entry; 
  errno = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (errno != 0) {
      perror("readdir");
      closedir(dir);
      return EXIT_FAILURE;
    }

    printf("Name: %s\n", entry->d_name);
    printf("Inode: %d\n", entry->d_ino);
    printf("   Type:  ");

    switch (entry->d_type) {
      case DT_UNKNOWN: printf("Unknown\n"); break;
      case DT_FIFO: printf("FIFO\n"); break;
      case DT_CHR: printf("Character Device\n"); break;
      case DT_DIR: printf("Directory\n"); break;
      case DT_BLK: printf("Block Device\n"); break;
      case DT_REG: printf("Regular file\n"); break;
      case DT_LNK: printf("Sym link\n"); break;
      case DT_SOCK: printf("Socker\n"); break;
      default: printf("Unknown\n"); break;
    }
    printf("\n");

    errno = 0;
  }

  closedir(dir);


  return EXIT_SUCCESS;
}
