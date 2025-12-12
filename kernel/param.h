#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

// Scheduling
#define TIME_SLICE_TICKS 5   // time slice per RUNNING process, in timer ticks
#define PRI_MIN        0
#define PRI_DEFAULT    1
#define PRI_MAX        3

// Memory watermarks（pages）用于低内存提醒
#define MEM_LOW_WATERMARK_PAGES      64
#define MEM_CRIT_WATERMARK_PAGES     32

