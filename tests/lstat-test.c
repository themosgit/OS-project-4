#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

/* C program to test functionality of lstat() system call */

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  const char *filename = argv[1];
  struct stat file_stat;
  if (lstat(filename, &file_stat) == -1) {
    perror("lstat");
    exit(EXIT_FAILURE);
  }

  printf("filename = %s\n", filename);
  printf("Type : ");
  if (S_ISREG(file_stat.st_mode)) {
    printf("regular file\n");
  } else if (S_ISDIR(file_stat.st_mode)) {
    printf("directory\n");
  } else if (S_ISLNK(file_stat.st_mode)) {
    printf("sym-link\n");
  } else if (S_ISFIFO(file_stat.st_mode)) {
    printf("fifo pipe\n");
  } else if (S_ISSOCK(file_stat.st_mode)) {
    printf("socket\n");
  } else if (S_ISCHR(file_stat.st_mode)) {
    printf("character device\n");
  } else if (S_ISBLK(file_stat.st_mode)) {
    printf("block device\n");
  } else {
    printf("unknown\n");
  }

  printf("Size: %ld\n", file_stat.st_size);
  printf("Inode: %ld\n", file_stat.st_ino);
  printf("Links: %ld\n", file_stat.st_nlink);
  printf("Permissions: %o\n", file_stat.st_mode & 0777);
  printf("Owner id: %d\n", file_stat.st_uid);
  printf("Group id: %d\n", file_stat.st_gid);

  char time_str[100];

  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_atime));
  printf("Access time: %s\n", time_str);

  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
  printf("Modify time: %s\n", time_str);

  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_ctime));
  printf("Change time: %s\n", time_str);

  return EXIT_SUCCESS;
}
