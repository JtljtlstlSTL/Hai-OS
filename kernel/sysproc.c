#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "hai_sysinfo.h"

extern struct proc proc[NPROC];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

uint64
sys_setpriority(void)
{
  int pid, prio;
  argint(0, &pid);
  argint(1, &prio);
  return setpriority(pid, prio);
}

uint64
sys_getpriority(void)
{
  int pid;
  argint(0, &pid);
  return getpriority(pid);
}

uint64
sys_klogctl(void)
{
  int level;
  argint(0, &level);
  return klog_set_level(level);
}

uint64
sys_sysinfo(void)
{
  uint64 uaddr;
  struct hai_sysinfo info;
  argaddr(0, &uaddr);

  uint total = 0, freep = 0;
  kalloc_stats(&total, &freep);
  info.total_pages = total;
  info.free_pages = freep;
  info.pressure_pct = kalloc_pressure_percent();
  info.ticks = ticks;
  info.log_level = klog_get_level();

  int procs = 0;
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state != UNUSED)
      procs++;
    release(&p->lock);
  }
  info.procs = procs;

  if(copyout(myproc()->pagetable, uaddr, (char*)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

static void
fill_procinfo(struct hai_procinfo *dst, struct proc *p)
{
  dst->pid = p->pid;
  dst->state = p->state;
  dst->priority = p->priority;
  dst->rtime = p->rtime;
  dst->sched_cnt = p->sched_cnt;
  dst->page_faults = p->page_faults;
  safestrcpy(dst->name, p->name, sizeof(dst->name));
}

uint64
sys_schedinfo(void)
{
  uint64 uaddr;
  struct hai_schedinfo info;
  memset(&info, 0, sizeof(info));
  argaddr(0, &uaddr);

  info.ticks = ticks;

  struct proc *p;
  int idx = 0;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    switch(p->state){
      case RUNNABLE: info.runnable++; break;
      case RUNNING:  info.running++; break;
      case SLEEPING: info.sleeping++; break;
      case ZOMBIE:   info.zombies++; break;
      case USED:     info.used++; break;
      default: break;
    }
    if(p->state != UNUSED && idx < HAI_MAX_PROCSNAPSHOT){
      fill_procinfo(&info.procs[idx], p);
      idx++;
    }
    release(&p->lock);
  }
  info.nreturned = idx;

  if(copyout(myproc()->pagetable, uaddr, (char*)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}

uint64
sys_vmstat(void)
{
  uint64 uaddr;
  struct hai_vmstat st;
  memset(&st, 0, sizeof(st));
  argaddr(0, &uaddr);

  kalloc_stats((uint *)&st.total_pages, (uint *)&st.free_pages);
  st.pressure_pct = kalloc_pressure_percent();

  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state != UNUSED)
      st.page_faults += p->page_faults;
    release(&p->lock);
  }

  if(copyout(myproc()->pagetable, uaddr, (char*)&st, sizeof(st)) < 0)
    return -1;
  return 0;
}

uint64
sys_devinfo(void)
{
  uint64 uaddr;
  argaddr(0, &uaddr);
  struct hai_devinfo info;
  memset(&info, 0, sizeof(info));
  driver_snapshot(&info);
  if(copyout(myproc()->pagetable, uaddr, (char*)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}

static int
fetch_argv(uint64 uargv, char **argv)
{
  int i;
  uint64 uarg;
  memset(argv, 0, sizeof(char*) * MAXARG);
  for(i = 0;; i++){
    if(i >= MAXARG)
      return -1;
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      return -1;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      return -1;
  }
  return 0;
}

static void
free_argv(char **argv)
{
  for(int i = 0; i < MAXARG; i++){
    if(argv[i])
      kfree(argv[i]);
  }
}

uint64
sys_spawn(void)
{
  char path[MAXPATH], *argv[MAXARG];
  uint64 uargv;

  argaddr(1, &uargv);
  if(argstr(0, path, MAXPATH) < 0)
    return -1;
  if(fetch_argv(uargv, argv) < 0){
    free_argv(argv);
    return -1;
  }

  int pid = kfork();
  if(pid < 0){
    free_argv(argv);
    return -1;
  }

  if(pid == 0){
    if(kexec(path, argv) < 0){
      free_argv(argv);
      kexit(-1);
    }
    // kexec 不返回，防御性返回
    free_argv(argv);
    kexit(-1);
  }

  free_argv(argv);
  return pid;
}

uint64
sys_eventfd(void)
{
  return -1; // 占位：未实现 eventfd
}

uint64
sys_timerfd(void)
{
  return -1; // 占位：未实现 timerfd
}

uint64
sys_dmesg(void)
{
  return -1; // 占位：未实现 dmesg 缓冲输出
}
