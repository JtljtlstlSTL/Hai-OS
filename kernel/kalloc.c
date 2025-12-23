// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint total_pages;     // 初始化时可分配的总页数
  uint free_pages;      // 当前空闲页数
  int low_warned;       // 低水位提醒是否已发
  int crit_warned;      // 临界水位提醒是否已发
  int oom_warned;       // OOM 是否已提醒
  int ready;            // 完成初始化后开启双重释放检查
} kmem;

// 每个物理页的引用计数，按页号索引。
static ushort refcnt[(PHYSTOP) / PGSIZE];

static inline int
pa_to_idx(uint64 pa)
{
  return pa / PGSIZE;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.total_pages = 0;
  kmem.free_pages = 0;
  kmem.low_warned = 0;
  kmem.crit_warned = 0;
  kmem.oom_warned = 0;
  kmem.ready = 0;
  freerange(end, (void*)PHYSTOP);
  kmem.ready = 1;
  klog(LOG_INFO, "Hai-OS kmem ready: total=%u free=%u pages", kmem.total_pages, kmem.free_pages);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    kmem.total_pages++;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int idx;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run*)pa;
  idx = pa_to_idx((uint64)pa);

  acquire(&kmem.lock);
  if(kmem.ready && refcnt[idx] == 0)
    panic("kfree double");

  if(refcnt[idx] > 1){
    refcnt[idx]--;
    release(&kmem.lock);
    return;
  }

  // Fill with junk to catch dangling refs when the page is truly freed.
  memset(pa, 1, PGSIZE);

  refcnt[idx] = 0;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.free_pages++;
  if(kmem.free_pages > MEM_LOW_WATERMARK_PAGES){
    kmem.low_warned = 0;
  }
  if(kmem.free_pages > MEM_CRIT_WATERMARK_PAGES){
    kmem.crit_warned = 0;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int idx = -1;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    idx = pa_to_idx((uint64)r);
    refcnt[idx] = 1;
  }
  if(r && kmem.free_pages > 0)
    kmem.free_pages--;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  // 低内存提醒（一次性告警，避免刷屏）
  if(r){
    if(kmem.free_pages <= MEM_CRIT_WATERMARK_PAGES && !kmem.crit_warned){
      kmem.crit_warned = 1;
      klog(LOG_WARN, "Hai-OS kmem critically low: free=%u pages", kmem.free_pages);
    } else if(kmem.free_pages <= MEM_LOW_WATERMARK_PAGES && !kmem.low_warned){
      kmem.low_warned = 1;
      klog(LOG_INFO, "Hai-OS kmem low: free=%u pages", kmem.free_pages);
    }
  } else {
    if(!kmem.oom_warned){
      kmem.oom_warned = 1;
      klog(LOG_ERR, "Hai-OS kmem exhausted: free=%u pages", kmem.free_pages);
    }
  }
  return (void*)r;
}

// 增加物理页的引用计数，用于 COW 共享。
int
kaddref(uint64 pa)
{
  int idx;
  if(pa >= PHYSTOP)
    return -1;
  idx = pa_to_idx(pa);
  acquire(&kmem.lock);
  if(refcnt[idx] == 0){
    release(&kmem.lock);
    return -1;
  }
  refcnt[idx]++;
  release(&kmem.lock);
  return refcnt[idx];
}

int
krefcount(uint64 pa)
{
  int idx;
  if(pa >= PHYSTOP)
    return 0;
  idx = pa_to_idx(pa);
  acquire(&kmem.lock);
  int c = refcnt[idx];
  release(&kmem.lock);
  return c;
}

// 查询内存统计，供内核/后续 syscall 使用
int
kalloc_stats(uint *total, uint *free)
{
  acquire(&kmem.lock);
  if(total) *total = kmem.total_pages;
  if(free) *free = kmem.free_pages;
  release(&kmem.lock);
  return 0;
}

// 返回当前内存压力百分比（0-100），供调度/策略决策。
int
kalloc_pressure_percent(void)
{
  int percent = 0;
  acquire(&kmem.lock);
  if(kmem.total_pages)
    percent = (int)(((kmem.total_pages - kmem.free_pages) * 100) / kmem.total_pages);
  release(&kmem.lock);
  return percent;
}
