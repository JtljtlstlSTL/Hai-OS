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
