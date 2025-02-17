#include "include/myz.h"

/* handles the command prompt for myz
* calls proper myz functions based on flags.*/

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
      assert(argc >= 3);
      myzExtract(&argv[2], argc - 2);
      break;
    case 'd':
      assert(argc >= 4);
      myzDelete(&argv[2], argc - 2);
      break;
    case 'm':
      assert(argc == 3);
      myzMetadata(argv[2]);
      break;
    case 'q':
      assert(argc >=4);
      myzQuery(&argv[2], argc - 2);
      break;
    case 'p':
      assert(argc == 3);
      myzPrint(argv[2]);
      break;
    default:
      printf("Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> <list-of-files/dirs>\n", argv[0]); 
      break;
  }
  return EXIT_SUCCESS;
}
