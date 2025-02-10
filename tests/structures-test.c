#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 4096

typedef enum {
  TFILE,
  TDIR,
  TSYMLINK,
  TUNKNOWN
}filetype;

typedef enum {
  TRUE,
  FALSE,
  MAYBE
} compressed;


struct header {
  long int size;
  long int myz_list;
  int number_of_nodes;
};

struct myz_node {
  compressed gzip;
  long int location;
  long int size;
  filetype type;
  int uid;
  int gid;
  int persmissions;
  char time_str[100];
  char name[100];
};


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input/files>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct myz_node node;
  struct header head;
  head.size = sizeof(struct header) + sizeof(struct myz_node);
  head.myz_list = sizeof(struct header);
  head.number_of_nodes = 1;
  strcpy(node.name, argv[1]);
  
  node.gzip = FALSE;

  struct stat statcheck;
  if (lstat(node.name, &statcheck) == -1) {
    perror("lstat");
    exit(EXIT_FAILURE);
  }

  if (S_ISREG(statcheck.st_mode)) {
    node.type = TFILE;
  } else if (S_ISDIR(statcheck.st_mode)) {
    node.type = TDIR;
  } else if (S_ISLNK(statcheck.st_mode)) {
    node.type = TSYMLINK;
  } else {
    node.type = TUNKNOWN;
  }
  /* PLEASE PAY ATTENTION TO THOSE LINES*/
  node.size = statcheck.st_size;
  node.location = head.myz_list;
  head.myz_list += node.size;
  head.size += node.size;
  /*preferably they should be within a module*/



  node.persmissions = statcheck.st_mode & 0777;
  node.uid =  statcheck.st_uid;
  node.gid = statcheck.st_gid;
  
  strftime(node.time_str, sizeof(node.time_str), "%Y-%m-%d %H:%M:%S", localtime(&statcheck.st_atime));
  
  head.size = sizeof(node) + sizeof(head) + node.size;
  head.myz_list = sizeof(head) + node.size;
  printf("size of header: %ld, size of file: %ld, sizeof myz-list: %ld myz would be located at: %ld\n", sizeof(struct header), node.size, sizeof(struct myz_node), head.myz_list);

  int outfd = open("test.myz", O_WRONLY | O_CREAT | O_TRUNC, 0644);

  write(outfd, &head, sizeof(head));

  int infd = open("node.name", O_RDONLY);
  char buffer[node.size];
  read(infd, buffer, node.size); 
  write(outfd, buffer, node.size);
  close(infd); 
  write(outfd, &node, sizeof(node));
  close(outfd);

  int myzfd = open("test.myz", O_RDONLY);
  struct header test_head; 
  struct myz_node test_node;


  read(myzfd, &test_head, sizeof(test_head));

  printf("Data from header Size of myz file:%ld, where myz-list is located:%ld\n", test_head.size, test_head.myz_list);
  lseek(myzfd, test_head.myz_list, SEEK_SET);


  read(myzfd, &test_node, sizeof(test_node));


  printf("Now showing data stored in myz node that was retrieved from .myz file\n");
  printf("Name of file: %s\n", test_node.name);
  if (test_node.type == TFILE) {
    printf("Regular file\n");
  } else if (test_node.type == TDIR) {
    printf("Directory\n");
  } else if (test_node.type == TSYMLINK) {
    printf("Symbolic link\n");
  } else {
    printf("Unknown\n");
  }
  if (test_node.gzip == TRUE) {
    printf("File has been compresed using gzip\n");
  } else {
    printf("File has not been compressed\n");
  }
  printf("Size of file: %ld bytes\n", test_node.size);
  printf("File persmissions: %o\n", test_node.persmissions);
  printf("User id: %d Group id: %d\n", test_node.uid, test_node.gid);
  printf("Access time: %s\n", test_node.time_str);

  close(myzfd);

  return EXIT_SUCCESS;
}
