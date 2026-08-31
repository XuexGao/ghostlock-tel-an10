// SPDX-License-Identifier: GPL-2.0
// 偏移提取: 每个 offsetof 编码成一个文件域数组的元素个数,
// aarch64-linux-gnu-nm -S 直接读符号 SIZE 列。
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/cred.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/pipe_fs_i.h>
#include <linux/rtmutex.h>
#include <linux/rbtree.h>
#include <linux/seccomp.h>
#include <linux/thread_info.h>
#include <asm/thread_info.h>
#include "rtmutex_common.h"

#define OFF(sym, field, tag) \
  char ksz_##tag[__builtin_offsetof(sym, field)];
#define SIZE(sym, tag) \
  char ksz_sz_##tag[sizeof(sym)];

/* task_struct */
OFF(struct task_struct, thread_info, task_thread_info)
OFF(struct task_struct, state, task_state)
OFF(struct task_struct, stack, task_stack)
OFF(struct task_struct, usage, task_usage)
OFF(struct task_struct, flags, task_flags)
OFF(struct task_struct, on_rq, task_on_rq)
OFF(struct task_struct, on_cpu, task_on_cpu)
OFF(struct task_struct, prio, task_prio)
OFF(struct task_struct, normal_prio, task_normal_prio)
OFF(struct task_struct, policy, task_policy)
OFF(struct task_struct, sched_class, task_sched_class)
OFF(struct task_struct, se, task_se)
OFF(struct task_struct, dl.deadline, task_dl_deadline)
OFF(struct task_struct, sched_task_group, task_task_group)
OFF(struct task_struct, tasks, task_tasks)
OFF(struct task_struct, pid, task_pid)
OFF(struct task_struct, tgid, task_tgid)
OFF(struct task_struct, real_parent, task_real_parent)
OFF(struct task_struct, parent, task_parent)
OFF(struct task_struct, group_leader, task_group_leader)
OFF(struct task_struct, mm, task_mm)
OFF(struct task_struct, real_cred, task_real_cred)
OFF(struct task_struct, cred, task_cred)
OFF(struct task_struct, comm, task_comm)
OFF(struct task_struct, seccomp, task_seccomp)
OFF(struct task_struct, cpu, task_cpu)
OFF(struct task_struct, on_cpu, task_on_cpu2)
OFF(struct task_struct, atomic_flags, task_atomic_flags)
OFF(struct task_struct, files, task_files)
OFF(struct task_struct, nsproxy, task_nsproxy)
OFF(struct task_struct, signal, task_signal)
OFF(struct task_struct, fs, task_fs)
OFF(struct task_struct, pi_lock, task_pi_lock)
OFF(struct task_struct, pi_waiters, task_pi_waiters)
OFF(struct task_struct, pi_top_task, task_pi_top_task)
OFF(struct task_struct, pi_blocked_on, task_pi_blocked_on)
#ifdef CONFIG_HW_VIP_THREAD
OFF(struct task_struct, static_vip, task_static_vip)
OFF(struct task_struct, vip_entry, task_vip_entry)
OFF(struct task_struct, enqueue_time, task_enqueue_time)
#endif
#ifdef CONFIG_HUAWEI_SCHED_VIP
OFF(struct task_struct, vip_prio, task_vip_prio)
OFF(struct task_struct, hisi_vip_entry, task_hisi_vip_entry)
OFF(struct task_struct, hisi_vip_last_queued, task_hisi_vip_last_queued)
#endif
#ifdef CONFIG_HW_QOS_THREAD
OFF(struct task_struct, proc_qos, task_proc_qos)
OFF(struct task_struct, thread_qos, task_thread_qos)
OFF(struct task_struct, trans_flags, task_trans_flags)
#endif
SIZE(struct task_struct, task)

/* thread_info (arm64) */
OFF(struct thread_info, flags, ti_flags)
OFF(struct thread_info, addr_limit, ti_addr_limit)
OFF(struct thread_info, preempt_count, ti_preempt_count)
SIZE(struct thread_info, ti)

/* cred */
OFF(struct cred, usage, cred_usage)
OFF(struct cred, uid, cred_uid)
OFF(struct cred, gid, cred_gid)
OFF(struct cred, suid, cred_suid)
OFF(struct cred, sgid, cred_sgid)
OFF(struct cred, euid, cred_euid)
OFF(struct cred, egid, cred_egid)
OFF(struct cred, fsuid, cred_fsuid)
OFF(struct cred, fsgid, cred_fsgid)
OFF(struct cred, securebits, cred_securebits)
OFF(struct cred, cap_inheritable, cred_cap_inh)
OFF(struct cred, cap_permitted, cred_cap_prm)
OFF(struct cred, cap_effective, cred_cap_eff)
OFF(struct cred, cap_bset, cred_cap_bset)
OFF(struct cred, cap_ambient, cred_cap_amb)
OFF(struct cred, jit_keyring, cred_jit_keyring)
OFF(struct cred, session_keyring, cred_session_keyring)
OFF(struct cred, process_keyring, cred_process_keyring)
OFF(struct cred, thread_keyring, cred_thread_keyring)
OFF(struct cred, request_key_auth, cred_request_key_auth)
OFF(struct cred, security, cred_security)
OFF(struct cred, user, cred_user)
OFF(struct cred, user_ns, cred_user_ns)
OFF(struct cred, group_info, cred_group_info)
SIZE(struct cred, cred)

/* rt_mutex */
OFF(struct rt_mutex, wait_lock, rtmtx_wait_lock)
OFF(struct rt_mutex, waiters, rtmtx_waiters)
OFF(struct rt_mutex, owner, rtmtx_owner)
SIZE(struct rt_mutex, rt_mutex)

/* rt_mutex_waiter */
OFF(struct rt_mutex_waiter, tree_entry, waiter_tree_entry)
OFF(struct rt_mutex_waiter, pi_tree_entry, waiter_pi_tree_entry)
OFF(struct rt_mutex_waiter, task, waiter_task)
OFF(struct rt_mutex_waiter, lock, waiter_lock)
OFF(struct rt_mutex_waiter, prio, waiter_prio)
OFF(struct rt_mutex_waiter, deadline, waiter_deadline)
SIZE(struct rt_mutex_waiter, waiter)

/* rbtree */
OFF(struct rb_node, __rb_parent_color, rb_parent_color)
OFF(struct rb_node, rb_right, rb_right)
OFF(struct rb_node, rb_left, rb_left)
OFF(struct rb_root_cached, rb_root, rbroot_root)
OFF(struct rb_root_cached, rb_leftmost, rbroot_leftmost)

/* mm_struct */
OFF(struct mm_struct, mmap, mm_mmap)
OFF(struct mm_struct, pgd, mm_pgd)
OFF(struct mm_struct, mm_count, mm_mm_count)
OFF(struct mm_struct, saved_auxv, mm_saved_auxv)
OFF(struct mm_struct, exe_file, mm_exe_file)
OFF(struct mm_struct, start_stack, mm_start_stack)
OFF(struct mm_struct, owner, mm_owner)
SIZE(struct mm_struct, mm)

/* pipe */
OFF(struct pipe_inode_info, mutex, pipe_mutex)
OFF(struct pipe_inode_info, wait, pipe_wait)
OFF(struct pipe_inode_info, nrbufs, pipe_nrbufs)
OFF(struct pipe_inode_info, curbuf, pipe_curbuf)
OFF(struct pipe_inode_info, buffers, pipe_buffers)
OFF(struct pipe_inode_info, bufs, pipe_bufs)
SIZE(struct pipe_inode_info, pipe_inode_info)
OFF(struct pipe_buffer, page, pipebuf_page)
OFF(struct pipe_buffer, offset, pipebuf_offset)
OFF(struct pipe_buffer, len, pipebuf_len)
OFF(struct pipe_buffer, ops, pipebuf_ops)
OFF(struct pipe_buffer, flags, pipebuf_flags)
OFF(struct pipe_buffer, private, pipebuf_private)
SIZE(struct pipe_buffer, pipe_buffer)

/* file_operations */
OFF(struct file_operations, llseek, fops_llseek)
OFF(struct file_operations, read, fops_read)
OFF(struct file_operations, write, fops_write)
OFF(struct file_operations, read_iter, fops_read_iter)
OFF(struct file_operations, write_iter, fops_write_iter)
OFF(struct file_operations, poll, fops_poll)
OFF(struct file_operations, unlocked_ioctl, fops_unlocked_ioctl)
OFF(struct file_operations, compat_ioctl, fops_compat_ioctl)
OFF(struct file_operations, mmap, fops_mmap)
OFF(struct file_operations, open, fops_open)
OFF(struct file_operations, release, fops_release)
OFF(struct file_operations, splice_read, fops_splice_read)
OFF(struct file_operations, show_fdinfo, fops_show_fdinfo)
SIZE(struct file_operations, fops)

/* seccomp */
OFF(struct seccomp, mode, seccomp_mode)
OFF(struct seccomp, filter, seccomp_filter)

/* page */
OFF(struct page, flags, page_flags)
OFF(struct file, f_op, file_f_op)
OFF(struct file, f_inode, file_f_inode)
OFF(struct file, private_data, file_private_data)
OFF(struct file, f_flags, file_f_flags)
SIZE(struct page, page)

/* list_head 基础 */
OFF(struct list_head, next, list_next)
OFF(struct list_head, prev, list_prev)
