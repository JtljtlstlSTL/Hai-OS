# Hai-OS 变更与使用指引（阶段 1-8）

## 内核与平台
- 启动与日志：BSS 提前清零、按 PGSIZE 设置启动栈；Hai-OS 风格 klog（含 hart/时间戳/等级），ASCII Logo 仅打印一次。
- 调度与进程：多档时间片与动态优先级；`setpriority/getpriority` 用户可调；tick 记账与饥饿保护。
- 内存：kalloc 水位与压力百分比、OOM 日志，kvminit 布局日志，page fault 计数可见于 sysinfo。
- 驱动框架（阶段 7）：统一注册/初始化/按 hart 初始化；内置 PLIC、virtio-blk；主/次核均通过框架完成 init。

## 文件系统 FSv2（阶段 6）
- 版式：超级块魔数 `HAIF`，版本/块大小指数/校验算法；inode 为 6 段 extent（NDIRECT/NINDIRECT=0），MAXFILE 重新定义。
- 逻辑：`bmap/itrunc/iupdate/ilock` 按 extent 运行；Adler-32 文件/目录校验（写后重算，整文件读验证，失败计入 `checksum_errors`）。
- 目录：inode 内置 DIRCACHE（最多 256 项），`dirlookup` 优先命中缓存，目录写/截断自动失效。
- 工具：`mkfs` 生成 FSv2 镜像，checksum_alg=Adler32，并为文件/目录预计算校验。
- 遥测：`statfs/fsinfo` 展示 FS 版本、块大小、校验算法、块/inode 使用、I/O 计数、校验错误。
- 增强：超级块/telemetry 增 data_csum/log_segments/quota 预留字段；新增 syscall `fverify` 与用户命令 `fverify` 做单文件校验重算验证。

## 系统调用 & 用户态（阶段 5, 8）
- Syscalls：`sysinfo`（物理页/压力/ticks/进程数/日志等级）、`setpriority`/`getpriority`、`klogctl`、`statfs`。
- 用户工具：
  - `sysinfo`：打印内核观测与调度控制示例。
  - `fsinfo`：打印 FSv2 元数据、块/inode 使用、校验状态。

## 用户空间体验（阶段 8 差异化）
- init：启动 shell 前输出 Hai-OS sysinfo + statfs 面板。
- sh：动态提示符 `[hai mem=XX% procs=YY lvl=Z]$`，内置命令：
  - `sysinfo`：查看内核观测。
  - `fsinfo`：查看 FSv2 统计。
  - `klog <lvl>`：切换内核日志等级（INFO/WARN/ERR/DEBUG）。
  - `prio <pid> <level>`：调整进程优先级（0-3）。
  - `fverify <path...>`：对目标文件重新计算校验并验证（调用内核 fverify）。

## 新命令使用示例
- 查看系统观测：
  - `sysinfo`
  - `fsinfo`
- 调整日志等级：
  - `klog 3`  切换到 DEBUG
- 调整进程优先级：
  - `prio 5 0` 将 pid 5 提升到最高优先级
- 常规 FS 验证：
  - `echo hi > /tmp/a && cat /tmp/a`
  - 运行 `fsinfo` 观察 used/free 与 checksum_errors
- 压力测试：
  - `stressfs` （观察 `fsinfo` 中 io_reads/io_writes 增长）

## 构建与运行
- 全量构建：`make clean && make kernel/kernel fs.img`
- 启动 QEMU：`make qemu`
-（如需回归）运行 `_usertests`：在 shell 执行 `usertests`

## 与原 xv6 的主要差异亮点
- FSv2 extent 布局 + Adler 校验 + DIRCACHE，加大文件上限，强化一致性与观测。
- 内核观测与调度可调 ABI（sysinfo/klogctl/setpriority）。
- 驱动框架统一初始化路径，便于扩展设备。
- 用户态提示符与内置命令突出可观测性与可调度性，远超原始 xv6 的最小交互壳。