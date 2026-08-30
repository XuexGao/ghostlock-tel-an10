// SPDX-License-Identifier: GPL-2.0
// configfs_buffer 偏移 (vendor 把定义移到了 fs/configfs/file.c, 这里内嵌)
#include <linux/fs.h>
#include <linux/configfs.h>
#include <linux/mutex.h>
#include <linux/module.h>

struct configfs_item_operations;
struct configfs_buffer {
	size_t			count;
	loff_t			pos;
	char			* page;
	struct configfs_item_operations	* ops;
	struct mutex		mutex;
	int			needs_read_fill;
	bool			read_in_progress;
	bool			write_in_progress;
	char			*bin_buffer;
	int			bin_buffer_size;
	int			cb_max_size;
	struct config_item	*item;
	struct module		*owner;
	union {
		struct configfs_attribute	*attr;
		struct configfs_bin_attribute	*bin_attr;
	};
};

#define OFF(sym, field, tag) char ksz_##tag[__builtin_offsetof(sym, field)];
#define SIZE(sym, tag) char ksz_sz_##tag[sizeof(sym)];

OFF(struct configfs_buffer, count, cfg_count)
OFF(struct configfs_buffer, pos, cfg_pos)
OFF(struct configfs_buffer, page, cfg_page)
OFF(struct configfs_buffer, ops, cfg_ops)
OFF(struct configfs_buffer, mutex, cfg_mutex)
OFF(struct configfs_buffer, needs_read_fill, cfg_needs_read_fill)
OFF(struct configfs_buffer, read_in_progress, cfg_read_in_progress)
OFF(struct configfs_buffer, write_in_progress, cfg_write_in_progress)
OFF(struct configfs_buffer, bin_buffer, cfg_bin_buffer)
OFF(struct configfs_buffer, bin_buffer_size, cfg_bin_buffer_size)
OFF(struct configfs_buffer, cb_max_size, cfg_cb_max_size)
OFF(struct configfs_buffer, item, cfg_item)
OFF(struct configfs_buffer, owner, cfg_owner)
SIZE(struct configfs_buffer, configfs_buffer)
