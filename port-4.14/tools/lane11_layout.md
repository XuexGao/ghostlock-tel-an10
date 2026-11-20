# lane 11 帧布局推导 (TEL-AN10 / 4.14.116)

本文件记录 `src/exploit/lane11.c` 里所有常量的**来源**，替代历史上
"靠 panic 猜偏移"的做法。全部依据 vendor 4.14.116 内核源码
(`XuexGao/TEL-AN10_opensource`, `Code_Opensource/kernel/`)。

## 1. 为什么需要这条 lane

`port-4.14` 原路线是 **perf 采样 + 页回收**（`reclaim`）把伪造 waiter 顶到
W0 内核栈上。本机 `CONFIG_SECURITY_PERF_EVENTS_RESTRICT=y`，shell 无
`CAP_PERFMON`，`perf_event_open` 不可用 —— 见 `DEBUG_STATE_2026-08-30.md`
第四/五轮结论：11 次探针全部卡在同一处。

参考项目 `GhostLock-MatePad11` 的 **rt_sigreturn lane（lane 11）** 不需要 perf：
信号帧里的 `fpsimd_context.vregs` 是 512B 的内核栈拷贝区，且
`restore_fpsimd_context()` 的 `__copy_from_user` 天然是"最后一个深帧拷贝"。
配合文件映射缺页，可以把伪造 waiter 稳在栈上并拉长消费窗口。

## 2. 帧几何（实测编译得，非估算）

由 `arch/arm64/include/uapi/asm/sigcontext.h` + `arch/arm64/kernel/signal.c`：

| 量 | 值 | 来源 |
|---|---|---|
| `sizeof(rt_sigframe)` | `0x1250` | siginfo(128) + ucontext |
| `offsetof(frame, uc)` | `0x80` | 内核 `regs[2] = &frame->uc` |
| `sigframe_size` | `0x480` | `init_user_layout` + `setup_sigframe_layout` |
| `uc_stack` 偏移 | `0x90` | `restore_altstack()` 读点 |
| `vregs` 偏移 | `0x260` | `restore_fpsimd_context()` 512B 拷贝目标 |

分配过程：
1. `init_user_layout()`：`size = offsetof(rt_sigframe, uc.uc_mcontext.__reserved) = 0x250`
2. `setup_sigframe_layout()`：先分配 `fpsimd_context`（`0x210`）→ `fpsimd_offset = 0x250`
3. 若有 `fault_code` 再加 `esr_context`（`0x10`）
4. `sigframe_alloc_end()` 加终止记录 `0x10`
5. 合计 `0x480`

`vregs` 在帧内 `0x260` 起，占 `[0x260, 0x460)`。

## 3. 为什么要把页边界卡在 uc_stack 与 vregs 之间

`sys_rt_sigreturn()` 的顺序（`arch/arm64/kernel/signal.c`）：

```
restore_sigframe()      -> restore_fpsimd_context()  : 512B 拷到 vregs (铺伪 waiter)
restore_altstack(&frame->uc.uc_stack)                : 读 uc_stack (低地址)
```

把含 `uc_stack` 的那一页用 `MAP_FIXED` 换成文件映射、并 `fadvise(DONTNEED)`
丢掉缓存后，第二步会缺页 → W0 睡 D 态 → 给消费线程留出窗口。
而 `vregs` 必须留在**下一页**（不被替换），才能保住刚铺好的伪 waiter。

约束：设 `frame_end & 0xFFF = fe_p`，需

```
fe_p + 0x90      < 0x1000      # uc_stack 在低页
fe_p + 0x260     >= 0x1000     # vregs 起点进高页
fe_p + 0x260+512 <= 0x2000     # vregs 512B 不逃出高页
```

`frame_end = frame_start - 0x480`，所以由 `frame_start & 0xFFF` 唯一决定。
取 `frame_start & 0xFFF = 0x220` → `fe_p = 0xda0`：

* `uc_stack` 页内 `0xe30`（低页，被打掉）
* `vregs` 页内 `0x1000`（高页，保留）

对应 `ss_size = 0x10230`；模块用 `0x10000` 作初值，运行时按实测帧地址
自动校准到该同余类（与参考项目 `setup_altstack()` 同一算法）。

## 4. 校验

```sh
cd port-4.14/tools
gcc -O2 -o lane11_layout lane11_layout.c && ./lane11_layout
```

输出应含：

```
sigframe_size (alloc)    = 0x480
offsetof(frame, uc)      = 0x80
uc_stack offset          = 0x90
vregs offset             = 0x260
WANT_PGOFF               = 0x220
```

真机侧可先跑无副作用的校准自检：

```sh
adb shell LD_PRELOAD=/data/local/tmp/ionstack_preload.so \
  IONSTACK_STAGE=lane11-check /data/local/tmp/ionstack_perf_target
```

它只建阻塞文件、校准备用栈、打一次信号帧，**不触发消费、不写内核**，
输出 `stage-lane11-check ... ok=1` 即说明帧布局推导在本机成立。

## 5. 仍需真机确认的量

* `L11_WAITER_IDX_DEFAULT`（`vregs` 内伪造 waiter 的 qword 下标）——
  取决于 4.14 的栈帧深度，可用 `IONSTACK_LANE11_WAITER_IDX` 覆盖后微调。
  这是唯一还要靠实测收敛的参数，与参考项目的 `DELTA`/`iov_off` 同性质。
* `selinux_state.enforcing` 精确偏移（`offset.h` 标了 `[VERIFY]`）。
