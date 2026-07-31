# ============================================================
#  CVE-2026-43499 GhostLock - HUAWEI TEL-AN10 (4.14) Build
#  Target: aarch64-linux-android (Termux clang / Android NDK)
#
#  默认使用 Termux 自带的 aarch64-linux-android-clang
#  (此环境: /data/data/com.termux/files/usr/bin/)
#  也可用 NDK: make NDK_ROOT=/path/to/android-ndk-r27
# ============================================================

# 配置 - 修改为你的NDK路径 (若使用 NDK)
NDK_ROOT ?= $(HOME)/android-ndk-r27
API ?= 33

# 优先使用 Termux 的 android clang, 否则回退 NDK
TERMUX_CLANG := /data/data/com.termux/files/usr/bin/aarch64-linux-android-clang
CC := $(if $(wildcard $(TERMUX_CLANG)),$(TERMUX_CLANG),$(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$(API)-clang)

CFLAGS := -O2 -Wall -Wextra -fPIE -pie -I.
LDFLAGS := -fPIE -pie -pthread

# 目标
TRIGGER_BIN := ghostlock_trigger
EXPLOIT_BIN := ghostlock_exploit

.PHONY: all trigger exploit clean push push_trigger push_exploit info

all: trigger exploit

# --- Trigger POC (漏洞验证) ---
trigger: $(TRIGGER_BIN)

$(TRIGGER_BIN): trigger.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Trigger 编译完成: $@"
	@echo "   用途: 验证漏洞是否存在 (预期内核 panic/重启)"

# --- Full Exploit (完整提权) ---
exploit: $(EXPLOIT_BIN)

$(EXPLOIT_BIN): exploit.c target.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Exploit 编译完成: $@"
	@echo "   用途: 完整提权测试 (DEBUG_MODE=1 默认开启)"

# --- 推送到设备 ---
push: push_trigger push_exploit

push_trigger: $(TRIGGER_BIN)
	adb push $(TRIGGER_BIN) /data/local/tmp/
	adb shell chmod 755 /data/local/tmp/$(TRIGGER_BIN)
	@echo "Trigger 已推送"
	@echo "   运行: adb shell /data/local/tmp/$(TRIGGER_BIN)"

push_exploit: $(EXPLOIT_BIN)
	adb push $(EXPLOIT_BIN) /data/local/tmp/
	adb shell chmod 755 /data/local/tmp/$(EXPLOIT_BIN)
	@echo "Exploit 已推送"
	@echo "   运行: adb shell /data/local/tmp/$(EXPLOIT_BIN)"

# --- 编译信息 ---
info:
	@echo "========================================"
	@echo " GhostLock Build Configuration"
	@echo "========================================"
	@echo " CC        = $(CC)"
	@echo " Target    = aarch64-linux-android"
	@echo "========================================"

clean:
	rm -f $(TRIGGER_BIN) $(EXPLOIT_BIN)
