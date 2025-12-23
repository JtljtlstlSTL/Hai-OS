#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

static void print_statfs(struct hai_statfs *st) {
  printf("Hai-OS FS info:\n");
  printf(" magic=0x%x size=%d data=%d inodes=%d\n", st->magic, st->size_blocks, st->data_blocks, st->inode_count);
  printf(" blocks: used=%d free=%d\n", st->used_blocks, st->free_blocks);
  printf(" inodes: used=%d free=%d\n", st->used_inodes, st->free_inodes);
  printf(" io: reads=%d writes=%d checksum_errors=%d\n", st->io_reads, st->io_writes, st->checksum_errors);
  printf(" features: journaling=%d checksum=%d quota=%d\n", st->has_journaling, st->has_checksum, st->has_quota);
  printf(" log: start=%d nblocks=%d\n", st->log_start, st->log_nblocks);
}

int
main(int argc, char **argv)
{
  struct hai_statfs st;
  if(statfs(&st) < 0) {
    printf("fsinfo: statfs failed\n");
    exit(1);
  }
  print_statfs(&st);
  exit(0);
}
