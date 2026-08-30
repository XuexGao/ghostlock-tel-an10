// SPDX-License-Identifier: GPL-2.0
// configfs_buffer 偏移 (私有头, 单独模块以便失败时不阻塞主表)
#include <linux/fs.h>
#include "configfs_internal.h"

#define OFF(sym, field, tag) char ksz_##tag[__builtin_offsetof(sym, field)];
#define SIZE(sym, tag) char ksz_sz_##tag[sizeof(sym)];

OFF(struct configfs_buffer, page, cfg_page)
OFF(struct configfs_buffer, needs_read_fill, cfg_needs_read_fill)
OFF(struct configfs_buffer, bin_buffer, cfg_bin_buffer)
OFF(struct configfs_buffer, bin_buffer_size, cfg_bin_buffer_size)
OFF(struct configfs_buffer, cb_max_size, cfg_cb_max_size)
SIZE(struct configfs_buffer, configfs_buffer)
