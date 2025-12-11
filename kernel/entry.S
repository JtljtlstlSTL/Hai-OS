#include "riscv.h"

        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # 为每个 hart 分配独立的启动栈（PGSIZE 对齐）。
        # stack0 在 start.c 中定义，按 hartid 偏移。
        la sp, stack0
        li a0, PGSIZE
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
        # jump to start() in start.c
        call start
spin:
        j spin
