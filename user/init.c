// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

static void
print_bootinfo(void)
{
  struct hai_sysinfo si;
  struct hai_statfs st;
  int hs = sysinfo(&si);
  int fs = statfs(&st);

  printf("Hai-OS init: userspace online\n");
  if(hs == 0)
    printf(" sysinfo: ticks=%d procs=%d mem_used=%d%% free_pages=%d total_pages=%d log_level=%d\n",
           (int)si.ticks, si.procs, si.pressure_pct, (int)si.free_pages, (int)si.total_pages, si.log_level);
  else
    printf(" sysinfo: unavailable\n");
  if(fs == 0)
    printf(" fsinfo: magic=0x%x ver=%d block=%d used=%d free=%d inodes used=%d free=%d checksum=%d\n",
           st.magic, st.version, st.block_size, st.used_blocks, st.free_blocks, st.used_inodes, st.free_inodes, st.has_checksum);
  else
    printf(" fsinfo: unavailable\n");
}

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  print_bootinfo();

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
