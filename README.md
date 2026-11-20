# GhostLock (CVE-2026-43499) — HUAWEI TEL-AN10 (Kernel 4.14) 构建仓库

利用 NebuSec IonStack GhostLock 漏洞链的 HUAWEI TEL-AN10 (Kirin 810,
4.14.116) 提权项目。**仅限自有机器的授权测试。**

## 目录

```
poc/        4.14 POC: trigger(漏洞验证) + exploit(提权), 偏移已按
            TEL-AN10 的 System.map + DWARF 适配完成
fullchain/  完整利用链参考 (xpad2-ionstack-poc, 4.19):
            slide KASLR 泄漏 + 物理读写 + cred 修补 + su daemon
            (移植到 4.14 进行中)
```

## GitHub Actions 构建

push 或手动触发 `build` workflow:

- **poc** 任务: 用 Android NDK r29 编译 `ghostlock_trigger` +
  `ghostlock_exploit`, 产物在 Artifacts 下载
- **fullchain** 任务: 编译完整链的设备端二进制 + Linux 主机控制器

本地构建:

```bash
# poc (Termux clang 或 NDK 均可)
make -C poc

# fullchain (需要 NDK r29)
make -C fullchain NDK_ROOT=/path/to/android-ndk-r29
```

## 设备使用

```bash
cd poc
./root.sh info      # 环境检查
./root.sh deploy    # 推送
CONFIRM=yes ./root.sh trigger   # 验证漏洞 (可能 panic 重启)
CONFIRM=yes ./root.sh debug     # 验证写原语
CONFIRM=yes ./root.sh root      # 提权
./root.sh verify
```

详细偏移表与注意事项见 `poc/README_4.14.md`。

## 移植进展 (4.14 完整链)

`port-4.14/` 是 4.19 完整链到 TEL-AN10 (4.14.116) 的移植。

- 当前状态与历史结论见 `port-4.14/PORT_4.14.md` 及
  `port-4.14/PROGRESS_*.md`、`DEBUG_STATE_2026-08-30.md`。
- **perf 页回收路线在本机不可用**（`CONFIG_SECURITY_PERF_EVENTS_RESTRICT=y`），
  已改用 rt_sigreturn 信号帧方案（lane 11），见
  `port-4.14/PROGRESS_2026-11-20.md` 与 `port-4.14/tools/lane11_layout.md`。

lane 11 帧布局由 vendor 4.14.116 源码实测推导（非猜测），可复核：

```sh
cd port-4.14/tools
gcc -O2 -o lane11_layout lane11_layout.c && ./lane11_layout
```

## 许可

- poc: 仓库内各文件原有许可
- fullchain: GPL-3.0-or-later (含 Apache-2.0 部分), 见 `fullchain/LICENSE`
