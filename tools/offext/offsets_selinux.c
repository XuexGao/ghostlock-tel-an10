// SPDX-License-Identifier: GPL-2.0
// selinux_state 布局 (SELinux 关闭阶段需要 enforcing 偏移)
#include <linux/types.h>
#include <linux/spinlock.h>
#include "security.h"

#define OFF(sym, field, tag) char ksz_##tag[__builtin_offsetof(sym, field)];
#define SIZE(sym, tag) char ksz_sz_##tag[sizeof(sym)];

OFF(struct selinux_state, enforcing, selinux_enforcing)
OFF(struct selinux_state, policycap, selinux_policycap)
OFF(struct selinux_state, deny_unknown, selinux_deny_unknown)
OFF(struct selinux_state, checkreqprot, selinux_checkreqprot)
SIZE(struct selinux_state, selinux_state)
