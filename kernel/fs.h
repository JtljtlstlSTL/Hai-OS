// On-disk file system format.
// Both the kernel and user programs use this header file.

#ifndef HAI_FS_H
#define HAI_FS_H

#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size (keep 1KB for now)

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
// Hai-OS FSv2 superblock
struct superblock {
  uint magic;        // FSMAGIC_v2
  uint version;      // layout version
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes
  uint nlog;         // Number of log blocks
  uint logstart;     // First log block
  uint inodestart;   // First inode block
  uint bmapstart;    // First free map block
  uint blocksz_exp;  // log2(block size)
  uint checksum_alg; // 0 none, 1 adler32 (planned)
};

#define FSMAGIC 0x48414946  // 'HAIF' Hai-OS FSv2

// Extent-based layout
#define NEXTENT   6
struct extent {
  uint start;   // start block
  uint len;     // length in blocks
};

// Legacy direct/indirect placeholders (compatibility with existing code paths)
#define NDIRECT   0
#define NINDIRECT 0

// On-disk inode structure (FSv2)
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links
  uint size;            // File size (bytes)
  uint checksum;        // placeholder for file-level checksum
  struct extent extents[NEXTENT];
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Extent-based MAXFILE: max logical blocks per file (cap extents to 1024 blocks each)
#define MAXFILE (NEXTENT * 1024)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

// The name field may have DIRSIZ characters and not end in a NUL
// character.
struct dirent {
  ushort inum;
  char name[DIRSIZ] __attribute__((nonstring));
};


// Hai-OS filesystem telemetry (Stage 6 scaffolding)
struct hai_statfs {
  uint magic;           // FSMAGIC
  uint version;         // FS layout version
  uint block_size;      // in bytes
  uint checksum_alg;    // 0 none, 1 adler32 (reserved)
  uint size_blocks;     // total blocks in image
  uint data_blocks;     // number of data blocks
  uint inode_count;     // total inodes
  uint free_blocks;     // free blocks (scanned bitmap)
  uint used_blocks;     // used blocks (data/log/inode/bmap)
  uint free_inodes;     // free inodes (type==0)
  uint used_inodes;     // used inodes

  // runtime I/O counters (from bio)
  uint io_reads;
  uint io_writes;
  uint checksum_errors; // runtime checksum verification failures (placeholder)

  // journaling/checksum/quota flags (placeholders for future Stage 6)
  uint has_journaling;  // 0/1
  uint has_checksum;    // 0/1
  uint has_quota;       // 0/1

  // log head/tail (placeholders, exposed by log.c)
  uint log_start;       // first log block
  uint log_nblocks;     // total log blocks
};

#endif // HAI_FS_H

