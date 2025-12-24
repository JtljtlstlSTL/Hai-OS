#pragma once
#include "types.h"

// Hai-OS 用户可见的系统观测数据，供 sysinfo 返回。
struct hai_sysinfo {
  uint64 total_pages;    // 物理页总数
  uint64 free_pages;     // 当前空闲页
  int    pressure_pct;   // 内存压力百分比（已用/总 * 100）
  uint64 ticks;          // 自启动以来的 tick 数
  int    procs;          // 非 UNUSED 进程数
  int    log_level;      // 当前内核日志等级
};

#define HAI_MAX_PROCSNAPSHOT 32

// 进程快照信息，用于 top/ps 等工具。
struct hai_procinfo {
  int pid;
  int state;
  int priority;
  uint64 rtime;
  uint64 sched_cnt;
  uint64 page_faults;
  char name[16];
};

struct hai_schedinfo {
  int runnable;
  int running;
  int sleeping;
  int zombies;
  int used;
  uint64 ticks;
  int nreturned;
  struct hai_procinfo procs[HAI_MAX_PROCSNAPSHOT];
};

struct hai_vmstat {
  uint64 total_pages;
  uint64 free_pages;
  int pressure_pct;
  uint64 page_faults;
};

#define HAI_MAX_DRIVERS 8

enum hai_driver_class {
  HAI_DRV_IRQ = 0,
  HAI_DRV_BLOCK = 1,
  HAI_DRV_SERIAL = 2,
  HAI_DRV_OTHER = 3,
};

struct hai_driver {
  char name[16];
  int cls;
  int inited;
  uint64 metric0;  // driver-specific metric #0
  uint64 metric1;  // driver-specific metric #1
  uint64 metric2;  // driver-specific metric #2
};

struct hai_devinfo {
  int count;
  struct hai_driver devs[HAI_MAX_DRIVERS];
};
