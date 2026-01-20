/*
 * convenient.h
 ***********************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 ************************************************************************
 * Brief Description:
 * A few convenience macros and routines..
 * Mostly for kernel-space usage, some for user-space as well.
 */
#ifndef __LKP_CONVENIENT_H__
#define __LKP_CONVENIENT_H__

#include <asm/param.h>		/* HZ */
#include <linux/sched.h>

#ifdef __KERNEL__
#include <linux/ratelimit.h>

/*
 *------------------------- Basic Instrumentation ---------------------
 * (the code-based debug-by-printing approach)
 *
 *** PLEASE READ this first ***
 *  1. The advent of the kernel's powerful *Dynamic Debug* functionality
 *     (CONFIG_DYNAMIC_DEBUG=y), renders our the MSG*() (and DBGPRINT()) macros defined
 *     here SUPERFLUOUS; they're not really required.
 *     Do use the kernel dynamic debug facility instead:
 *      https://www.kernel.org/doc/html/v6.1/admin-guide/dynamic-debug-howto.html#dynamic-debug
 *     (You might still find the QP*() macros useful, and the MSG*() ones useful
 *      for user space).
 *
 *  2. We can reduce the load, and increase readability, by using the trace_printk
 *  instead of printk. To see the trace_printk() output do:
 *     cat /sys/kernel/debug/tracing/trace
 *
 *  If we insist on using the regular printk, lets at least rate-limit it.
 *	For the programmers' convenience, this too is programatically controlled
 *	(by an integer var USE_RATELIMITING [default: On]).
 *
 * 3. Kernel module authors Note:
 *	To use the trace_printk(), pl #define the symbol USE_FTRACE_PRINT in your
 *	Makefile:
 *	 ccflags-y += -DUSE_FTRACE_PRINT
 *	If you do not do this, we will use the usual printk() .
 *
 * 4. To view kernel messages sent via:
 *	  printk's       : dmesg
 *    trace_printk's : cat /sys/kernel/debug/tracing/trace
 *	 Default: printk (with rate-limiting)
 */
/* Keep this defined to use the FTRACE-style trace_printk(), else will use
 * regular printk()
 */
//#define USE_FTRACE_BUFFER
#undef USE_FTRACE_BUFFER

#ifdef USE_FTRACE_BUFFER
#define DBGPRINT(string, args...)                          \
	trace_printk(string, ##args)
#else
#define DBGPRINT(string, args...) do {                     \
	int USE_RATELIMITING = 1;                          \
	if (USE_RATELIMITING) {                            \
		pr_info_ratelimited(string, ##args);       \
	}                                                  \
	else                                               \
		pr_info(string, ##args);                   \
} while (0)
#endif
#endif				/* #ifdef __KERNEL__ */

/*------------------------ MSG, QP ------------------------------------*/
#ifdef DEBUG
#ifdef __KERNEL__
#define MSG(string, args...) DBGPRINT("%s:%d : " string, __func__, __LINE__, ##args)
#else
#define MSG(string, args...) fprintf(stderr, "%s:%d : " string, __func__, __LINE__, ##args)
#endif

#ifdef __KERNEL__
#define MSG_SHORT(string, args...) DBGPRINT(string, ##args)
#else
#define MSG_SHORT(string, args...) fprintf(stderr, string, ##args)
#endif

// QP = Quick Print
#define QP MSG("\n")

#ifdef __KERNEL__
#ifndef USE_FTRACE_BUFFER
#define QPDS do {                 \
	MSG("\n");                \
	dump_stack();             \
} while (0)
#else
#define QPDS do {                 \
	MSG("\n");                \
	trace_dump_stack();       \
} while (0)
#endif
#endif

#ifdef __KERNEL__
#define HexDump(from_addr, len)   \
	print_hex_dump_bytes(" ", DUMP_PREFIX_ADDRESS, from_addr, len)
#endif
#else				/* #ifdef DEBUG */
#define MSG(string, args...)
#define MSG_SHORT(string, args...)
#define QP
#define QPDS
#endif

/*------------------------ SHOW_DELTA_{[bKMG*]} ------------------------
 * SHOW_DELTA_*(low, hi) :
 * Show the low val, high val and the delta (hi-low) in either bytes/KB/MB/GB,
 * as required.
 * Inspired from raspberry pi kernel src: arch/arm/mm/init.c:MLM()
 */
#define SHOW_DELTA_b(low, hi) (low), (hi), ((hi) - (low))
#define SHOW_DELTA_K(low, hi) (low), (hi), (((hi) - (low)) >> 10)
#define SHOW_DELTA_M(low, hi) (low), (hi), (((hi) - (low)) >> 20)
#define SHOW_DELTA_G(low, hi) (low), (hi), (((hi) - (low)) >> 30)
#define SHOW_DELTA_MG(low, hi) (low), (hi), (((hi) - (low)) >> 20), (((hi) - (low)) >> 30)

#if (BITS_PER_LONG == 64)
#define SHOW_DELTA_MGT(low, hi) (low), (hi), (((hi) - (low)) >> 20), (((hi) - (low)) >> 30), (((hi) - (low)) >> 40)
#else // 32-bit
#define SHOW_DELTA_MGT(low, hi) (low), (hi), (((hi) - (low)) >> 20), (((hi) - (low)) >> 30)
#endif

#ifdef __KERNEL__
/*------------------------ PRINT_CTX ---------------------------------*/
/*
 * An interesting way to print the context info; we mimic the kernel
 * Ftrace 'latency-format', printing the 'usual' 4 columns of kernel state info:
 *                       _-----=> irqs-off          [d]
 *                      / _----=> need-resched      [N]
 *                     | / _---=> hardirq/softirq   [H|h|s] [1]
 *                     || / _--=> preempt-depth     [#]
 *                     ||| /
 * CPU  TASK/PID       ||||  DURATION                  FUNCTION CALLS
 * |     |    |        ||||   |   |                     |   |   |   |
 *
 * [1] 'h' = hard irq is running ; 'H' = hard irq occurred inside a softirq]
 *
 * Sample output (via a debug printk;
 * also, as we're within a comment, we make / * ... * / into \* ... *\ ):
 *  CPU)  task_name:PID  | irqs,need-resched,hard/softirq,preempt-depth  \* func_name() *\
 *  001)  rdwr_drv_secret -4857   |  ...0   \* read_miscdrv_rdwr() *\
 *
 * (of course, above, we don't display the 'Duration' and 'Function Calls' fields)
 *
 * @@@ NOTE @@@
 * As we use pr_debug() to print this info, you will typically need to either:
 *  a) define the symbol DEBUG in the module/kernel (hard-coded)
 *  b) use the kernel's dynamic debug framework to see the debug prints (soft-coded, better!)
 */
#include <linux/sched.h>
#include <linux/interrupt.h>

/* 
 * We use a compat wrapper macro such that it uses in_hardirq() on newer
 * kernels and falls back to using in_irq() on older kernels. This avoids
 * embedding preprocessor directives inside a continued macro body (which
 * breaks the preprocessor)
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0)
#define IN_HARDIRQ_CHECK()  in_hardirq()
#else
#define IN_HARDIRQ_CHECK()  in_irq()
#endif

#define PRINT_CTX() do {                                                             \
	int PRINTCTX_SHOWHDR = 0;                                                    \
	char intr = '.';                                                             \
	if (!in_task()) {                                                            \
		if (IN_HARDIRQ_CHECK() && in_serving_softirq())                      \
			intr = 'H'; /* hardirq occurred inside a softirq  */         \
		else if (IN_HARDIRQ_CHECK())                                         \
			intr = 'h'; /* hardirq is running */                         \
		else if (in_serving_softirq())                                       \
			intr = 's';                                                  \
	}                                                                            \
	else                                                                         \
		intr = '.';                                                          \
									             \
	if (PRINTCTX_SHOWHDR == 1)                                                   \
		pr_debug("CPU)  task_name:PID  | irqs,need-resched,hard/softirq,preempt-depth  /* func_name() */\n"); \
	pr_debug(                                                                    \
	"%03d) %c%s%c:%d   |  "                                                      \
	"%c%c%c%u   "                                                                \
	"/* %s() */\n"                                                               \
	, raw_smp_processor_id(),                                                    \
	(!current->mm?'[':' '), current->comm, (!current->mm?']':' '), current->pid, \
	(irqs_disabled()?'d':'.'),                                                   \
	(need_resched()?'N':'.'),                                                    \
	intr,                                                                        \
	(preempt_count() && 0xff),                                                   \
	__func__                                                                     \
	);                                                                           \
} while (0)
#endif
/*
 * Interesting:
 * Above, I had to change the function smp_processor_id() to raw_smp_processor_id(); else,
 * on a DEBUG kernel (configured with many debug config options), the foll warnings
 * would ensue:
Oct 04 12:19:53 dbg-LKD kernel: BUG: using smp_processor_id() in preemptible [00000000] code: rdmem/12133
Oct 04 12:19:53 dbg-LKD kernel: caller is debug_smp_processor_id+0x17/0x20
Oct 04 12:19:53 dbg-LKD kernel: CPU: 0 PID: 12133 Comm: rdmem Tainted: G      D    O      5.10.60-dbg01 #1
Oct 04 12:19:53 dbg-LKD kernel: Hardware name: innotek GmbH VirtualBox/VirtualBox, BIOS VirtualBox 12/01/2006
Oct 04 12:19:53 dbg-LKD kernel: Call Trace:
Oct 04 12:19:53 dbg-LKD kernel:  dump_stack+0xbd/0xfa
...
 * This is caught due to the fact that, on a debug kernel, when the kernel config
 * CONFIG_DEBUG_PREEMPT is enabled, it catches the possibility that functions
 * like smp_processor_id() are called in an atomic context where sleeping / preemption
 * is disallowed! With the 'raw' version it works without issues (just as Ftrace does).
 */

/*------------------------ assert ---------------------------------------
 * Hey, careful!
 * Using assertions is great *but* be aware of traps & pitfalls:
 * http://blog.regehr.org/archives/1096
 *
 * The closest equivalent perhaps, to assert() in the kernel are the BUG(),
 * BUG_ON() and WARN(), WARN_ON() macros. Using BUG*() is _only_ for those
 * cases where recovery is impossible. WARN*() is usally considered a better
 * option. Pl see <asm-generic/bug.h> for details.
 *
 * Here, we just trivially emit a noisy printk to "warn" the dev/user
 * that the assertion failed.
 */
#ifdef __KERNEL__
#define assert(expr) do {                                                    \
if (!(expr)) {                                                               \
	pr_warn("********** Assertion [%s] failed! : %s:%s:%d **********\n", \
	#expr, __FILE__, __func__, __LINE__);                                \
}                                                                            \
} while (0)
#endif

/*------------------------ DELAY_LOOP --------------------------------*/
static inline void beep(int what)
{
#ifdef __KERNEL__
	pr_cont("%c", (char)what);
#else
#include <unistd.h>
	char buf = (char)what;
	(void)write(STDOUT_FILENO, &buf, 1);
#endif
}

/*
 * DELAY_LOOP macro
 * (Mostly) mindlessly loop, then print a char (via our beep() routine,
 * to emulate 'work' :-)
 * @val        : ASCII value to print
 * @loop_count : times to loop around
 */
#define DELAY_LOOP(val, loop_count)                                       \
{                                                                         \
	int c = 0, m;                                                     \
	unsigned int for_index, inner_index, x;                           \
									  \										\
	for (for_index = 0; for_index < loop_count; for_index++) {        \
		beep((val));                                              \
		c++;                                                      \
		for (inner_index = 0; inner_index < HZ; inner_index++) {  \
			for (m = 0; m < 50; m++);                         \
			x = inner_index / 2;                              \
		}                                                         \
	}                                                                 \
	/*printf("c=%d\n",c);*/                                           \
}
/*------------------------------------------------------------------------*/

#ifdef __KERNEL__
/*------------------------ SHOW_DELTA() macro -------------------------
 * Show the difference between the timestamps passed
 * Parameters:
 *  @later, @earlier : nanosecond-accurate timestamps
 * We expect that @later > @earlier
 *
 * Typical usage pattern:
 *	u64 t1, t2;
 *	t1 = ktime_get_real_ns();
 *	[ ... code whose time u want to measure ... ]
 *	t2 = ktime_get_real_ns();
 *	SHOW_DELTA(t2, t1);
 *
 * Hang on!
 * On ARM-32 (experienced this for the Beaglebone Black, arm-linux-gnueabi-gcc
 * (GCC) 10.5.0), the ktime_sub() and ktime_get_real_ns() routines causes the
 * compiler to throw this error:
...
 MODPOST <...>ds3231_i2c_drv/try/Module.symvers
ERROR: modpost: "__aeabi_uldivmod" [...ds3231_i2c_drv/try/ds3231t2.ko] undefined!
make[2]: *** [scripts/Makefile.modpost:123: ...ds3231_i2c_drv/try/Module.symvers] Error 1
...
 * Apparently, kernel routines like the ktime_sub(), ktime_get*(), etc are fine
 * on 64-bit but not on 32-bit!
 */
#if (BITS_PER_LONG == 64)
#include <linux/jiffies.h>
#include <linux/ktime.h>
#define SHOW_DELTA(later, earlier)  do {                                               \
if (time_after((unsigned long)later, (unsigned long)earlier)) {                        \
	s64 delta_ns = ktime_to_ns(ktime_sub(later, earlier));                         \
	pr_info("delta: %lld ns", delta_ns);                                           \
	if (delta_ns/1000 >= 1)                                                        \
		pr_cont(" (~ %lld us", delta_ns/1000);                                 \
	if (delta_ns/1000000 >= 1)                                                     \
		pr_cont(" ~ %lld ms", delta_ns/1000000);                               \
	if (delta_ns/1000 >= 1)                                                        \
		pr_cont(")\n");                                                        \
} else                                                                                 \
	pr_warn("SHOW_DELTA(): *invalid* earlier > later? (check order of params)\n"); \
} while (0)
#else   // 32-bit
	/* ktime_sub() not supported on 32-bit */
#define SHOW_DELTA(later, earlier)  \
	pr_warn("SHOW_DELTA(): ktime_sub() not supported on 32-bit\n")
#endif
#endif   /* #ifdef __KERNEL__ */
#endif   /* #ifndef __LKP_CONVENIENT_H__ */
