#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  if(argc < 2){
    printf("usage: fverify <path> [path...]\n");
    exit(1);
  }
  int ret = 0;
  for(int i = 1; i < argc; i++){
    int r = fverify(argv[i]);
    if(r == 0)
      printf("ok  %s\n", argv[i]);
    else{
      printf("FAIL %s\n", argv[i]);
      ret = 1;
    }
  }
  exit(ret);
}
