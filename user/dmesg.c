#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  int rc = dmesg();
  if(rc < 0){
    printf("dmesg: ring buffer not implemented; check console output.\n");
    if(argc == 2){
      int lvl = atoi(argv[1]);
      int prev = klogctl(lvl);
      printf("klog level set to %d (prev %d)\n", lvl, prev);
    }
  }
  exit(0);
}
