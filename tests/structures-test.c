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


typedef struct {
  long int size;
  long int myz_list;
  int number_of_nodes;
}Header;


typedef struct {
  compressed gzip;
  long int location;
  long int size;
  filetype type;
  int uid;
  int gid;
  int persmissions;
  char time_str[100];
  char name[100];
}myzNode;


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input/files>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  myzNode node;
  Header head;
  head.size = sizeof(myzNode) + sizeof(Header);
  head.myz_list = sizeof(Header);
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
  printf("Size of header: %ld\nSize of file: %ld\nSizeof myz-list: %ld\nMyz would be located at: %ld\n", sizeof(Header), node.size, sizeof(myzNode), head.myz_list);

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
  Header test_head; 
  myzNode test_node;


  read(myzfd, &test_head, sizeof(test_head));

  printf("Data from header Size of myz file:%ld\nWhere myz-list is located:%ld\n", test_head.size, test_head.myz_list);
  lseek(myzfd, test_head.myz_list, SEEK_SET);


  read(myzfd, &test_node, sizeof(test_node));


  printf("Now showing data stored in myz node\n");
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
