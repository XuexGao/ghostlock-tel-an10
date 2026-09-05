# TEL-AN10 GhostLock 调试状态快照 (2026-08-30 下午)

## 已完成验证 (今天)
1. **漏洞确认**: trigger → kernel_panic,fatal_exception 重启, CVE-2026-43499 存在 ✓
2. **构建环境恢复**: Termux clang 21 + 手动 sysroot (/tmp/build_android.sh)
   - 关键坑: 加 `-mno-outline-atomics` (clang21 outline atomics 引用
     __aarch64_ldadd4_acq_rel, Android10 bionic 无此符号 → 加载失败)
3. **结构体偏移全部实测确认** (GitHub Actions + vendor 源码):
   - 源码: https://github.com/XuexGao/TEL-AN10_opensource (4.14.116 vendor)
   - 方法: tools/offext/ 模块 (offsetof→符号大小) + 真机 /proc/config.gz
   - 工作流: .github/workflows/extract-offsets.yml (gh 已配好, 直接 dispatch)
   - 结果: offset.h 的 task/cred/rt_mutex/waiter/pipe/fops/configfs 偏移
     **全部正确** (pi_lock=0xa60 ✓, pi_blocked_on=0xa98 ✓, waiter 0x50 ✓)
   - 唯一错误: FAKE_TASK_TASK_GROUP_OFF 应为 0x530 (现 0x1d0, 落在 se 内,
     对当前路径良性)
   - thread_info = 0x18 (vendor 砍掉 task/cpu 成员), task_state=0x18
   - configfs_buffer 布局与 README 一致 (0x90)
4. **panic 边界精确定位** (O_SYNC trace):
   - 写路径全程干净: prep → w0 → EDEADLK → send_go → wake → recvmsg(off=0x30,
     EINVAL 预期) → waiter-route-done → main-sched-enter
   - **panic 在 sched_setattr 内部 = chain walk 期间**
   - SAFE_LOCK(owner=NULL, walk 只到[9]+wake) 也 panic → 不是 [10]-[12] 写
     逻辑的问题

## 已排除的假设
- 结构体偏移错误 ✗ (CI 实测全部吻合)
- outline atomics ✗ (已修)
- LTO/编译问题 ✗
- walk [7]/[9]/[11] 的 rb 树操作逻辑 ✗ (对照 4.14.116 源码逐步验证安全:
  pc=1 → BLACK root → insert break; loggers erase RED → 无 recolor 攀爬;
  单子节点 case rebalance=NULL)

## 当前头号嫌疑 (未验证)
**iovstack 落点**: iov_off=0x30 是 8/6 推算值 ("真机偏差需扫描"), 从未实测。
- iovstack 假设在链深 0x330, residual rt_waiter 在 0x300 (差 0x30)
- 若实际落点偏移 ±8/±0x10, fake waiter 字段错位 → walk 读到垃圾 task/lock
  → panic (与现象吻合)
- 边界检查限制 iov_off ≤ 0x30 (blob 128 - waiter 0x50)
- 已测: 0x40(OOB), 0x1c(8/21), 0x30(今天×3) 全 panic

## 下一步 (两条路)
A. **拿到真机内核二进制** (用户有 EMUI10 线刷包 → boot.img → Image):
   反汇编 sys_sendmsg 链 + futex_wait_requeue_pi 帧 → 精确算 iovstack
   与 rt_waiter 深度差 → 直接得出正确 iov_off (一次性, 无 panic 扫描)
   - 顺带验证 System.map、KASLR 线性映射模型、未来所有帧布局问题
B. **panic 扫描** (备选): diag 加读 LOCK_OFF+0x18 (lock waiters root,
   [7] 会写成栈地址 → 决定性标记), SAFE_LOCK 模式扫 iov_off
   0x00-0x30 step 4 (每个错误猜测 = 1 次重启)

## 环境备注
- adb: `adb -s emulator-5554` (多 transport, 192.168.0.101 unauthorized 忽略)
- 部署: /tmp/build_android.sh 重编 + push build/{ionstack_preload.so,
  ionstack_perf_target,su_daemon_aarch64_pie} → /data/local/tmp/
- 跑法: LD_PRELOAD=... IONSTACK_STAGE=slide IONSTACK_SLIDE_WRITE_PATH=1
  IONSTACK_KS_COLLISIONS=2 (放宽时序验证门) timeout 150 /data/local/tmp/ionstack_perf_target
- trace: /data/local/tmp/trace.txt (O_SYNC, panic 后存活)
- 孤儿进程清理: 大量 fork 的 futex spray 子进程会残留, 用 pidof+kill 清理,
  否则 ks 时序探测失效
- 设备后台: force-stop helium/bili 释放内存 (ks 探测敏感)
- 本预设禁止 rm: 设备/本机文件只能清空(truncate)或 mv, 不能删

## 第二轮结论 (2026-08-31)
**已证实为好的部分**:
- 页回收 + payload 落地: **perf 无关地验证通过**
  `cvalidate tee=4096 first=4096 avail=1000 mismatch=1000 ok=1`
  (开 IONSTACK_RECLAIM_SPLICE_ORDER_GATE=1 + HOLD_ALWAYS=1 +
   VALIDATE_CONTENT=1 + REQUIRE_CONTENT=1; HOLD_ALWAYS 是我加的解耦,
   原来 hold 被 order3_gate_ok(perf) 卡死 → 全程盲跑)
- 结构体偏移: CI 实测全部吻合 (仅 task_group 0x1d0→0x530, mm_owner 0x408→0x360 已修)
- task_struct.cpu=0x10c (vendor 有 THREAD_INFO_IN_TASK), 未被预置污染 → cpu_rq(0) 合法

**已排除**: S 值错误、栈覆盖错位、rb 树逻辑、NODE_SAFE 归零 waiter_task(已修)

**仍未解决**: walk 在 sched_setattr 内 panic, 五种组合全 panic:
  copy+default / copy+SAFE_LOCK / SKIP_PSELECT+SAFE_LOCK /
  SKIP_PSELECT+NODE_SAFE / +add_key 喷射
→ 悬垂 lock = &pi_state->pi_mutex (slot+0x10) 未被受控内容占据

**add_key 喷射不可行的原因**: user_key_payload 头占 0x18, 且 24+128=152 → kmalloc-256,
  完全错缓存。需要"整块 128 字节可控且常驻"的原语 (sendmsg cmsg 是对的, 但单次抢不过竞态)

**下一步关键**: 需要真机内核二进制 (线刷包里的 BOOT.img) 来
  1) 精确算 ___sys_sendmsg 帧 → iovstack 深度 → 正确 iov_off
  2) 精确算 futex_wait_requeue_pi 帧 → rt_waiter 深度
  3) 验证 pi_state/pi_mutex 偏移
  有了它就不必"一次 panic 换一个猜测"。

**流程教训**: 本 agent 跑在手机上, 手机 panic 会连带杀掉会话与后台任务;
  禁止用后台脚本跨重启; 每次设备探针都要前台阻塞并预期可能重启。

## 第三轮 (2026-08-31)
- 实现 32 路并发 ctl_buf sender 喷射 (自旋等待, 不睡眠不分配, 同内容)
  + 页内容验证门 ok=1 + NODE_SAFE(写目标在页内) → **仍然 panic**
  trace: cvalidate ok=1 / write-send-entry / (无 send-ret=阻塞中) /
         waiter-route-done / ctx / main-sched-enter → 死
- 逐行对照 4.14.116 rtmutex.c + rbtree 源码推演: NODE_SAFE 下 [7]/[9]/[11]/[12]
  所有写都落在 payload 页内 → 理论上不该 panic
  ⇒ 结论: **残余 waiter 的 lock 字段实际值 ≠ 我们的 ctl_buf**
     (pi_state 槽未被受控内容占据, 或 lock 根本不是 slot+0x10)
- 已试且失败: 单次 ctl_buf / 32 路 ctl_buf / add_key 喷射(错缓存 kmalloc-256)
- 不可行的"任意对齐"技巧: 把窗口填满 fake_lock 会让 prio 变成内核地址
  低 32 位(可能为负) → dl_prio() 为真 → 走 DL 路径崩

## 现在需要的东西 (二选一)
A) **真机内核二进制** (EMUI10 线刷包里的 BOOT.img → 解出 kernel Image)
   → 反汇编 ___sys_sendmsg / futex_wait_requeue_pi 帧, 精确算 iov_off;
   → 校验 futex_pi_state 真实布局与 pi_mutex 偏移;
   → 之后**一次探针**即可, 不再靠 panic 猜。
   放到 /home/tel-an10/ 下告诉我文件名即可。
B) 继续 panic 扫描 (每个候选 1 次重启, 约 5-13 次)

## 流程纪律 (重要)
- 本 agent 跑在手机上: 手机 panic = 我的会话+后台任务一起死
- 禁止后台脚本跨重启; 设备探针一律前台阻塞
- 关键诊断一律写 O_SYNC 的 /data/local/tmp/trace.txt (stdout 会被 panic 冲掉)
- 重启后 adb 需重新授权: 先 `adb connect 192.168.0.101:5555` 触发弹窗,
  用户在手机上点"一律允许"; emulator-5554 是可用 transport

## 第四轮 (2026-09-05) — 收敛结论
新增并验证的事实:
- 喷射后 trace 的 openat 会冲掉伪造 waiter (代码原注释已警告"openat 链帧深 0x280+
  污染 [0x2b1,0x300]") → 已移除喷射后的所有深帧 syscall (waiter-route-done 等)
- 落点无关喷射 (窗口每 qword = fake_lock) 已实现: sendmsg 版 + pselect 版(nfds=8192,
  4KB 窗口, 裸 SYS_pselect6 绕 bionic FD_SETSIZE)
- CONFIG_CHECKPOINT_RESTORE 未开 → 官方 prctl(PR_SET_MM_MAP) user_auxv 路线在本机不可用
- CONFIG_SLAB_FREELIST_RANDOM+HARDENED; add_key 被 SELinux 拒 (errno 13, 已单独验证)
- 32 路并发 ctl_buf sender 仍抢不到 pi_state 槽

累计 10+ 次探针全部在同一位置 panic: main-sched-enter 之后 (chain walk 内)。
已排除: 结构体偏移(CI 实测全对) / S 值(NODE_SAFE 下写目标在页内仍崩) /
栈覆盖错位(落点无关填充仍崩) / 喷射后污染(已清除仍崩) / CPU 亲和 / randstruct

⇒ 剩余唯一未验证的关键假设: **真机内核栈上, 喷射窗口与残余 rt_waiter 的实际相对位置**,
   以及 **悬垂 lock 指向的对象当前真实状态**。这两点无法从源码/270/210 包推得
   (真机是 212 构建: Nov 12 2021; 手上包是 Oct 18 2021 与 Aug 16 2022, 布局已不同)。

## 已固化的可复用资产
- GitHub Actions: extract-offsets 工作流 (vendor 源码 + 真机 config → 精确偏移, 可复跑)
- tools/offext/ 提取模块 + phone_config.txt
- firmware/update-extractor.py 解 UPDATE.APP (已验证可导 BOOT.img)
- build-env/{build_android.sh, rebuild_sysroot.sh, scan_iov.sh} + 禁 rm 注意事项
- 页回收 perf 无关验证门 (SPLICE_ORDER_GATE + HOLD_ALWAYS + VALIDATE/REQUIRE_CONTENT)

## 第五轮 (2026-09-05) — 停止上机试错
- 全行 pselect 喷射实现有 bug: dup2 覆盖 3..n-1 会**踩掉 T1 的 socketpair fd**
  → trace 出现 `write-send-ret errno=104 (ECONNRESET)` → ctl_buf 被释放
  → bootreason 从 `kernel_panic,bug` 变成 `kernel_panic,fatal_exception` (失败模式改变)
- 独立验证 (零风险): pselect6(nfds=320..8192) 一律立即返回 EBADF
  → 位图里未打开的 fd 被判错误; 但 copy_from_user 在报错前已完成 (内容确实上栈)
- 累计 13 次上机探针, 全部在 main-sched-enter 之后 panic, 改变喷射方式只改变
  panic 子类型, 未收敛

结论: 该写路径需要**真机那颗内核 (212 / 2021-11-12) 的二进制**来定帧布局;
210(Oct18)/270(Aug2022) 包经 banner 偏移校验均与真机布局不同, 不能替代。
