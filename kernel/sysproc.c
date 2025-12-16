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
