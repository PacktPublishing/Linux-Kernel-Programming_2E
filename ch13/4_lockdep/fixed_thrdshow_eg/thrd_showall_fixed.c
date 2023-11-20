/*
 * ch13/3_lockdep/buggy_thrdshow_eg/thrd_showall_buggy.c
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
 * This kernel module is based upon our earlier kernel module from Ch 6:
 *  ch6/foreach/thrd_showall/thrd_showall.c. 
 * Earlier (in the buggy version), we refactored it to use the
 * get_task_comm() routine to retrieve the name of the thread, it's buggy!
 * The bug turns out to be a recursive locking issue, *detected by lockdep*.
 *
 * So now let's fix it.
 * Here, we fix the deadlock by first unlocking the relevant lock (struct
 * task_struct alloc_lock), then calling get_task_comm() and then locking it.
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
MODULE_DESCRIPTION("LKP 2E book: ch13/4_lockdep/fixed_thrdshow_eg/:"
" FIX to the earlier buggy demo to display all threads by iterating over the task list");
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
#if 0
	/* The tasklist_lock reader-writer spinlock for the task list 'should'
	 * be used here, but, it's not exported, hence unavailable to our
	 * kernel module. So, as this qualifies as a mostly-read scenario,
	 * we should use the best option: RCU! 
	 * For the sake of a pedantic example, to trigger a (self) deadlock bug
	 * here, we don't; we use task_lock().
	 * Read Ch 13 for the details!
	 */
	rcu_read_lock(); /* This triggers off an RCU read-side critical section; ensure
			  * you are non-blocking within it! */
#endif
	do_each_thread(g, t) {     /* 'g' : process ptr; 't': thread ptr */
		get_task_struct(t);	/* take a reference to the task struct */
		task_lock(t);  /*** task lock taken here! ***/

		snprintf(buf, BUFMAX-1, "%6d %6d ", g->tgid, t->pid);
		/* task_struct addr and kernel-mode stack addr */
		snprintf(tmp, TMPMAX-1, "  0x%px", t);
		strncat(buf, tmp, TMPMAX);
		snprintf(tmp, TMPMAX-1, "  0x%px", t->stack);
		strncat(buf, tmp, TMPMAX);

        /* In the 'buggy' ver of this code, LOCKDEP did catch a deadlock here !!
         * (at the point that get_task_comm() was invoked).
         * The reason's clear: get_task_comm() attempts to take the very same lock
         * that we just took above via task_lock(t);  !! This is obvious self-deadlock...
         * So, we fix it here by first unlocking it, calling get_task_comm(), and
         * then re-locking it.
         */
                task_unlock(t);
                get_task_comm(tasknm, t);
                task_lock(t);

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
		task_unlock(t);
		put_task_struct(t);	/* release reference to the task struct */
	} while_each_thread(g, t);
#if 0
	/* <same as above, reg the RCU synchronization for the task list> */
	rcu_read_unlock();
#endif

	return total;
}

static int __init thrd_showall_buggy_init(void)
{
	int total;

	pr_info("inserted\n");
	total = showthrds_buggy();
	pr_info("total # of threads on the system: %d\n",
		total);

	return 0;		/* success */
}
static void __exit thrd_showall_buggy_exit(void)
{
	pr_info("removed\n");
}

module_init(thrd_showall_buggy_init);
module_exit(thrd_showall_buggy_exit);
