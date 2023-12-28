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
#include "../../../convenient.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP 2E book: ch13/3_lockfree/thrdshowall_rcu/:"
" Proper fix to the earlier buggy demos to display all threads by iterating over the task list, using RCU");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int showthrds_rcu(void)
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
	struct task_struct *g_rcu, *t_rcu;  /* 'g_rcu' : process ptr; 't_rcu': thread ptr */

	pr_info("%s", hdr);

	rcu_read_lock(); /* This triggers off an RCU read-side critical section; ensure
			  * you are non-blocking within it!
			  */
	/*
	 * Interesting: we use the do_each_thread(g, t) to iterate over every
	 * thread alive. Internally, it becomes:
	 * include/linux/sched/signal.h:
	 * ...
	 * #define do_each_thread(g, t) \
	 *	for (g = t = &init_task ; (g = t = next_task(g)) != &init_task ; ) do
	 *
	 * #define while_each_thread(g, t) \
	 *	while ((t = next_thread(t)) != g)
	 *  ...
	 * Notice how both macros invoke the next_*() macro to iterate to the next
	 * list member. Now, the implemetation of next_{task|thread}() uses
	 * include/linux/rculist.h:list_entry_rcu(), which is the RCU protected means
	 * of access!
	 * As it's comment says: '...  This primitive may safely run concurrently with
	 * the _rcu list-mutation primitives such as list_add_rcu() as long as it's
	 * guarded by rcu_read_lock(). ...'
	 */
	do_each_thread(g, t) {     /* 'g' : process ptr; 't': thread ptr */
		g_rcu = rcu_dereference(g);
		t_rcu = rcu_dereference(t);

		get_task_struct(t_rcu);	/* take a reference to the task struct */

		snprintf_lkp(buf, BUFMAX-1, "%6d %6d ", g_rcu->tgid, t_rcu->pid);
		/* task_struct addr and kernel-mode stack addr */
		snprintf_lkp(tmp, TMPMAX-1, "  0x%px", t_rcu);
		strncat(buf, tmp, TMPMAX);
		snprintf_lkp(tmp, TMPMAX-1, "  0x%px", t_rcu->stack);
		strncat(buf, tmp, TMPMAX);
		get_task_comm(tasknm, t_rcu);

		if (!g_rcu->mm)	// kernel thread
			snprintf_lkp(tmp, sizeof(tasknm)+4, " [%16s]", tasknm);
		else
			snprintf_lkp(tmp, sizeof(tasknm)+4, "  %16s ", tasknm);
		strncat(buf, tmp, TMPMAX);

		/* Is this the "main" thread of a multithreaded process?
		 * We check by seeing if (a) it's a userspace thread,
		 * (b) it's TGID == it's PID, and (c), there are >1 threads in
		 * the process.
		 * If so, display the number of threads in the overall process
		 * to the right..
		 */
		nr_thrds = get_nr_threads(g_rcu);
		if (g_rcu->mm && (g_rcu->tgid == t_rcu->pid) && (nr_thrds > 1)) {
			snprintf_lkp(tmp, TMPMAX-1, " %3d", nr_thrds);
			strncat(buf, tmp, TMPMAX);
		}

		snprintf_lkp(tmp, 2, "\n");
		strncat(buf, tmp, 2);
		pr_info("%s", buf);

		total++;
		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));
		put_task_struct(t_rcu);	/* release reference to the task struct */
	} while_each_thread(g, t);
	rcu_read_unlock();

	return total;
}

static int __init thrd_showall_rcu_init(void)
{
	int total;

	pr_info("inserted\n");
	total = showthrds_rcu();
	pr_info("total # of threads on the system: %d\n", total);

	return 0;		/* success */
}
static void __exit thrd_showall_rcu_exit(void)
{
	pr_info("removed\n");
}

module_init(thrd_showall_rcu_init);
module_exit(thrd_showall_rcu_exit);
