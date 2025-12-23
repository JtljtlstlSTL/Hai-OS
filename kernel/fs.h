// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

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

