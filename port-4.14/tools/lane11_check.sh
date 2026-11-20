#!/bin/sh
# lane 11 真机零风险自检: 只验证帧布局, 不触发消费, 不写内核。
# 用法: ./lane11_check.sh [设备序列号]
set -e
SERIAL="${1:-}"
ADB="adb"
[ -n "$SERIAL" ] && ADB="adb -s $SERIAL"

echo "[*] 设备:"; $ADB shell getprop ro.product.model; $ADB shell uname -r
echo "[*] 推送..."
$ADB push build/ionstack_preload.so /data/local/tmp/ >/dev/null
$ADB push build/ionstack_perf_target /data/local/tmp/ >/dev/null
$ADB shell chmod 755 /data/local/tmp/ionstack_perf_target

echo "[*] 跑 lane11-check (不会 panic)..."
$ADB shell "LD_PRELOAD=/data/local/tmp/ionstack_preload.so \
  IONSTACK_STAGE=${STAGE:-lane11-check} /data/local/tmp/ionstack_perf_target" || true

echo
echo "[*] 期望: 'lane11 altstack calibrated size=0x10230 pgoff=0x220' 且 ok=1"
