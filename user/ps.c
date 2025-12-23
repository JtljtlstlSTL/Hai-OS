#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/hai_sysinfo.h"
#include "user/user.h"

static const char *state_name(int st)
{
  switch(st){
    case 1: return "USED";
    case 2: return "SLEEP";
    case 3: return "RUNN";
    case 4: return "RUN";
    case 5: return "ZOMB";
    default: return "UNUSED";
  }
}

int
main(int argc, char **argv)
{
  struct hai_schedinfo sc;
  if(schedinfo(&sc) < 0){
    printf("ps: schedinfo failed\n");
    exit(1);
  }

  printf("PID  PRIO STATE  RTIME  SCHED  PF  NAME\n");
  for(int i = 0; i < sc.nreturned; i++){
    struct hai_procinfo *p = &sc.procs[i];
    printf("%-4d %-4d %-6s %-6d %-6d %-3d %s\n",
           p->pid, p->priority, state_name(p->state), (int)p->rtime, (int)p->sched_cnt, (int)p->page_faults, p->name);
  }

  exit(0);
}
