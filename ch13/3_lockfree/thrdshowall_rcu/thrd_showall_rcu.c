/*
 * ch13/3_lockfree/thrdshowall_rcu/thrd_showall_rcu.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 13 : Kernel Synchronization, Part 2
 ****************************************************************
 * Brief Description:
 * This kernel module is based upon our earlier kernel module here:
 *  ch13/4_lockdep/fixed_thrdshow_eg/
 * We had "fixed it" by using the task_{un}lock() pair of APIs to provide
 * synchronization. However, this wasn't an ideal solution as it introduced
 * the possibility of a race, plus, the task_{un}lock() routines employ a
 * spinlock that’s effective for only some of the task structure members.
 *
 * So, here, we do a proper fix by employing lockfree RCU! It's as-is very
 * efficient for read-mostly situations, which this certainly qualifies as.
 * Moreover, here, as we don’t ever modify any task structure's content,
 * we even eliminate the need for write protection via a spinlock.
 *
 * For details, please refer the book, Ch 13.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>     /* current() */
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/signal.h>
#endif

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP 2E book: ch13/3_lockfree/thrdshowall_rcu/:"
" Proper fix to the earlier buggy demos to display all threads by iterating over the task list, using RCU");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int showthrds_buggy(void)
{
	struct task_struct *g, *t;  /* 'g' : process ptr; 't': thread ptr */
	int nr_thrds = 1, total = 0;
#define BUFMAX		256
#define TMPMAX		128
	char buf[BUFMAX], tmp[TMPMAX], tasknm[TASK_COMM_LEN];
	const char hdr[] =
"--------------------------------------------------------------------------------\n"
"    TGID   PID         current        stack-start      Thread Name   MT? # thrds\n"
"--------------------------------------------------------------------------------\n";

	pr_info("%s", hdr);

	rcu_read_lock(); /* This triggers off an RCU read-side critical section; ensure
			  * you are non-blocking within it!
			  */
	do_each_thread(g, t) {     /* 'g' : process ptr; 't': thread ptr */
		get_task_struct(t);	/* take a reference to the task struct */

		snprintf(buf, BUFMAX-1, "%6d %6d ", g->tgid, t->pid);
		/* task_struct addr and kernel-mode stack addr */
		snprintf(tmp, TMPMAX-1, "  0x%px", t);
		strncat(buf, tmp, TMPMAX);
		snprintf(tmp, TMPMAX-1, "  0x%px", t->stack);
		strncat(buf, tmp, TMPMAX);

		if (!g->mm)	// kernel thread
			snprintf(tmp, sizeof(tasknm)+3, " [%16s]", tasknm);
		else
			snprintf(tmp, sizeof(tasknm)+3, "  %16s ", tasknm);
		strncat(buf, tmp, TMPMAX);

		/* Is this the "main" thread of a multithreaded process?
		 * We check by seeing if (a) it's a userspace thread,
		 * (b) it's TGID == it's PID, and (c), there are >1 threads in
		 * the process.
		 * If so, display the number of threads in the overall process
		 * to the right..
		 */
		nr_thrds = get_nr_threads(g);
		if (g->mm && (g->tgid == t->pid) && (nr_thrds > 1)) {
			snprintf(tmp, TMPMAX-1, " %3d", nr_thrds);
			strncat(buf, tmp, TMPMAX);
		}

		snprintf(tmp, 2, "\n");
		strncat(buf, tmp, 2);
		pr_info("%s", buf);

		total++;
		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));
		put_task_struct(t);	/* release reference to the task struct */
	} while_each_thread(g, t);
	rcu_read_unlock();

	return total;
}

static int __init thrd_showall_buggy_init(void)
{
	int total;

	pr_info("inserted\n");
	total = showthrds_buggy();
	pr_info("total # of threads on the system: %d\n", total);

	return 0;		/* success */
}
static void __exit thrd_showall_buggy_exit(void)
{
	pr_info("removed\n");
}

module_init(thrd_showall_buggy_init);
module_exit(thrd_showall_buggy_exit);
