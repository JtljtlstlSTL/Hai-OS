#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/hai_sysinfo.h"
#include "user/user.h"

static const char *level_name(int lvl){
  switch(lvl){
    case 0: return "INFO";
    case 1: return "WARN";
    case 2: return "ERROR";
    case 3: return "DEBUG";
    default: return "UNKNOWN";
  }
}

int
main(int argc, char **argv)
{
  struct hai_sysinfo info;
  int rc = sysinfo(&info);
  if(rc < 0){
    printf("sysinfo: failed\n");
    exit(1);
  }

  printf("Hai-OS sysinfo:\n");
  printf("  pages: total=%d free=%d pressure=%d%%\n", (int)info.total_pages, (int)info.free_pages, info.pressure_pct);
  printf("  procs: %d\n", info.procs);
  printf("  ticks: %d\n", (int)info.ticks);
  printf("  klog : level=%s(%d)\n", level_name(info.log_level), info.log_level);

  if(argc == 3 && strcmp(argv[1], "klog") == 0){
    int lvl = atoi(argv[2]);
    int prev = klogctl(lvl);
    printf("klog level changed from %s(%d) to %s(%d)\n", level_name(prev), prev, level_name(lvl), lvl);
  }

  if(argc == 4 && strcmp(argv[1], "setprio") == 0){
    int pid = atoi(argv[2]);
    int pr = atoi(argv[3]);
    if(setpriority(pid, pr) < 0)
      printf("setpriority pid=%d pr=%d failed\n", pid, pr);
    else
      printf("setpriority pid=%d pr=%d ok\n", pid, pr);
  }

  exit(0);
}
