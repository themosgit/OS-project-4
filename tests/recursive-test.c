#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAXPATH 256

int processDir(const char *path){
  DIR *dir;
  struct dirent *entry;
  struct stat filestat;
  char full_path[MAXPATH];

  dir = opendir(path);
  if (dir == NULL) {
    perror("opendir");
    exit(EXIT_FAILURE);
  }

  while ((entry = readdir(dir)) != NULL) {
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (lstat(full_path, &filestat) == -1) {
      perror("lstat");
      continue;
    }

    printf("Path: %s\n", full_path);
    printf("Inode: %lu\n", filestat.st_ino);
    printf("Type: ");
    if (S_ISREG(filestat.st_mode)) {
      printf("Regular file\n");
    } else if (S_ISDIR(filestat.st_mode)) {
      printf("Directory\n");
    } else if (S_ISLNK(filestat.st_mode)) {
      printf("Symbolic link\n");
    } else {
      printf("Other\n");
    }
    
    printf("\n");

    if (S_ISDIR(filestat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      if (processDir(full_path) != 0 ) {
        closedir(dir);
        exit(EXIT_FAILURE);
      }
    }
  }

  closedir(dir);
  return 0; 
}


int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <Directory>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (processDir(argv[1]) != 0) {
    fprintf(stderr, "Error proccessing directory %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}

