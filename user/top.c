#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/hai_sysinfo.h"
#include "user/user.h"

static void
swap(struct hai_procinfo *a, struct hai_procinfo *b)
{
  struct hai_procinfo tmp = *a;
  *a = *b;
  *b = tmp;
}

static void
sort_by_rtime(struct hai_procinfo *arr, int n)
{
  for(int i = 0; i < n; i++){
    int maxidx = i;
    for(int j = i + 1; j < n; j++){
      if(arr[j].rtime > arr[maxidx].rtime)
        maxidx = j;
    }
    if(maxidx != i)
      swap(&arr[i], &arr[maxidx]);
  }
}

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
  struct hai_sysinfo si;
  struct hai_vmstat vm;
  struct hai_schedinfo sc;
  int rc_si = sysinfo(&si);
  int rc_vm = vmstat(&vm);
  int rc_sc = schedinfo(&sc);
  if(rc_si < 0 || rc_vm < 0 || rc_sc < 0){
    printf("top: failed syscalls\n");
    exit(1);
  }

  printf("Hai-OS top (one-shot)\n");
  printf("  mem: total=%d free=%d pressure=%d%% faults=%d\n", (int)vm.total_pages, (int)vm.free_pages, vm.pressure_pct, (int)vm.page_faults);
  printf("  procs: total=%d runnable=%d running=%d sleep=%d zombie=%d ticks=%d\n",
         si.procs, sc.runnable, sc.running, sc.sleeping, sc.zombies, (int)sc.ticks);

  int n = sc.nreturned;
  if(n > HAI_MAX_PROCSNAPSHOT)
    n = HAI_MAX_PROCSNAPSHOT;
  sort_by_rtime(sc.procs, n);

  int show = n < 8 ? n : 8;
  printf("\nPID  PRIO STATE  RTIME  SCHED  PF  NAME\n");
  for(int i = 0; i < show; i++){
    struct hai_procinfo *p = &sc.procs[i];
    printf("%-4d %-4d %-6s %-6d %-6d %-3d %s\n",
           p->pid, p->priority, state_name(p->state), (int)p->rtime, (int)p->sched_cnt, (int)p->page_faults, p->name);
  }

  exit(0);
}
