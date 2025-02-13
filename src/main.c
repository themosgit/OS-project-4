#include "include/myz.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> <list-of-files/dirs>\n", argv[0]);
    return EXIT_FAILURE;
  }
  switch (argv[1][1]) {
    case 'c': 
      if (argv[2][1] == 'j') {
        assert(argc >= 5);
        myzInit(argv[3]);
        myzInsert(&argv[3], argc - 3, true);
      } else {
        assert(argc >= 4);
        myzInit(argv[2]);
        myzInsert(&argv[2], argc - 2, false);
      } 
      break;
    case 'a':
      if (argv[2][1] == 'j') {
        assert(argc >= 5);
        myzInsert(&argv[3], argc - 3, true);
      } else {
        assert(argc >= 4);
        myzInsert(&argv[2], argc - 2, false);
      }
      break;
    case 'x':
      //extract function 
      break;
    case 'd':
      //delete function
      break;
    case 'm':
      //metadata print for all files
      break;
    case 'q':
      //query for a file return yes or no  
      break;
    case 'p':
      //print the myz file structure in a readdable manner
      break;
    default:
      printf("Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> <list-of-files/dirs>\n", argv[0]); 
      break;
  }
  return EXIT_SUCCESS;
}
