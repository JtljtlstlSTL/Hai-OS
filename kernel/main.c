#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

static const char *haios_version = "Hai-OS/alpha";

static void boot_banner(void);
static void print_ascii_logo_once(void);
static void primary_init_sequence(void);
static void secondary_init_sequence(void);

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    primary_init_sequence();
  } else {
    secondary_init_sequence();
  }

  scheduler();        
}

static void
boot_banner(void)
{
  // 启动第一条品牌化日志，附带版本与时间戳，做系统指纹。
  uint64 boot_cycle = r_time();
  klog(LOG_INFO, "Hai-OS bootstrap sequence engaged version=%s boot_cycle=%p",
       haios_version, (void*)boot_cycle);
}

static void
print_ascii_logo_once(void)
{
  static int printed = 0;
  if(printed)
    return;
  printed = 1;

  // 只打印一次 ASCII 花体 Logo，用于可视化确认启动成功。
  printf("\n");
  printf("  _    _       _ _           _                     \n");
  printf(" | |  | |     (_) |         | |                    \n");
  printf(" | |__| | __ _ _| |__   __ _| |__   __ _ _ __ __ _ \n");
  printf(" |  __  |/ _` | | '_ \\ / _` | '_ \\ / _` | '__/ _` |\n");
  printf(" | |  | | (_| | | |_) | (_| | |_) | (_| | | | (_| |\n");
  printf(" |_|  |_|\\__,_|_|_.__/ \\__,_|_.__/ \\__,_|_|  \\__,_|\n");
  printf("\n");
}

static void
primary_init_sequence(void)
{
  consoleinit();
  printfinit();
  boot_banner();

  // 内存初始化：物理分配器与内核页表
  klog(LOG_INFO, "memory: kinit");
  kinit();         // physical page allocator
  klog(LOG_INFO, "memory: pressure=%d%%", kalloc_pressure_percent());

  klog(LOG_INFO, "memory: kvminit");
  kvminit();       // create kernel page table
  kvminithart();   // turn on paging

  // 进程与陷阱初始化
  klog(LOG_INFO, "proc: procinit");
  procinit();      // process table

  klog(LOG_INFO, "trap: trapinit");
  trapinit();      // trap vectors
  trapinithart();  // install kernel trap vector

  // 中断控制器
  klog(LOG_INFO, "irq: plicinit");
  plicinit();      // set up interrupt controller
  plicinithart();  // ask PLIC for device interrupts

  // 文件系统相关子系统
  klog(LOG_INFO, "fs: buffer cache");
  binit();         // buffer cache
  klog(LOG_INFO, "fs: inode table");
  iinit();         // inode table
  klog(LOG_INFO, "fs: file table");
  fileinit();      // file table
  klog(LOG_INFO, "fs: virtio disk");
  virtio_disk_init(); // emulated hard disk

  // 启动第一个用户进程
  klog(LOG_INFO, "user: init process");
  userinit();      // first user process

  __sync_synchronize();
  started = 1;
  klog(LOG_INFO, "Hai-OS primary hart is live");
}

static void
secondary_init_sequence(void)
{
  while(started == 0)
    ;
  __sync_synchronize();
  // 从核复用主核已完成的初始化，仅做本核的硬件配置。
  klog(LOG_INFO, "hart %d starting", cpuid());
  kvminithart();    // turn on paging
  trapinithart();   // install kernel trap vector
  plicinithart();   // ask PLIC for device interrupts
  klog(LOG_INFO, "hart %d entered scheduler", cpuid());
  if(cpuid() == 2)
    print_ascii_logo_once();
}
