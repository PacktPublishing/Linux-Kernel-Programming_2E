/*
 * ch6/foreach/thrd_showall/thrd_showall.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 6 : Kernel Internals Essentials - Processes and Threads
 ****************************************************************
 * Brief Description:
 * This kernel module iterates over the task structures of all *threads*
 * currently alive on the box, printing out some details.
 * We use the do_each_thread() { ... } while_each_thread() macros to do
 * so.
 *
 * For details, please refer the book, Ch 6.
 */
//#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>	/* current() */
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/signal.h>
#endif

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
("LKP 2E:ch6/foreach/thrd_showall: demo to display all threads by iterating over the task list");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

/* Display just CPU 0's idle thread, i.e., the pid 0 task,
 * the (terribly named) 'swapper/n'; n = 0, 1, 2,...
 * Again, init_task is always the task structure of the first CPU's
 * idle thread, i.e., we're referencing swapper/0.
 */
static inline void disp_idle_thread(void)
{
	struct task_struct *t = &init_task;

	/* We know that the swapper is a kernel thread */
	pr_info("%8d %8d  0x%px 0x%px [%16s]\n",
		//"%8d %8d  0x%px 0x%px",
		t->pid, t->pid, t, t->stack, t->comm);
}

static int showthrds(void)
{
	struct task_struct *p = NULL, *t = NULL;	/* 'p' : process ptr; 't': thread ptr */
	int nr_thrds = 1, total = 1;	/* total init to 1 for the idle thread */
	/* Not using dynamic allocation here as we haven't covered it yet... :-/
	 * Careful - don't overflow the small kernel space stack!
	 */
#define TMPMAX		128
	char tmp1[TMPMAX], tmp2[TMPMAX], tmp3[TMPMAX];
	const char hdr[] =
	    "------------------------------------------------------------------------------------------\n"
	    "    TGID     PID         current           stack-start         Thread Name     MT? # thrds\n"
	    "------------------------------------------------------------------------------------------\n";

	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	memset(tmp3, 0, sizeof(tmp3));
	pr_cont("%s", hdr);
	disp_idle_thread();

	/*
	 * (The stuff mentioned on locking here can be skipped on first reading; you can check
	 * it out once you read the materials on kernel synchronization / locking, Ch 12 and
	 * Ch 13; will leave it to you...).
	 * The do_each_thread() / while_each_thread() is a pair of macros that iterates over
	 * _all_ task structures in memory.
	 * The task structs are global of course; this implies we should hold a lock of some
	 * sort while working on them (even if only reading!). So, doing
	 *  read_lock(&tasklist_lock);
	 *  [...]
	 *  read_unlock(&tasklist_lock);
	 * BUT, tasklist_lock isn't exported and is thus unavailable to modules.
	 * So, using an RCU read lock is indicated here... read about RCU and
	 * it's usage in Ch 13.
	 */
	rcu_read_lock(); /* This triggers off an RCU read-side critical section; ensure
			  * you are non-blocking within it! */
	do_each_thread(p, t) {	/* 'p' : process ptr; 't': thread ptr */
		get_task_struct(t);	/* take a reference to the task struct */

		/* Grab the following fields from the task struct:
		 * tgid, pid, task_struct addr, kernel-mode stack addr
		 */
		snprintf(tmp1, TMPMAX - 1, "%8d %8d  0x%px 0x%px",
			 p->tgid, t->pid, t, t->stack);

		/* One might question why we don't use the get_task_comm() to obtain
		 * the task's name here; the short reason: it causes a deadlock! We
		 * shall explore this (and how to avoid it) in some detail in Ch 13 -
		 * Kernel Synchronization Part 2. For now, we just do it the simple way
		 */
		if (!p->mm)	// kernel thread?
			snprintf(tmp2, TMPMAX - 1, " [%16s]", t->comm);
		else
			snprintf(tmp2, TMPMAX - 1, "  %16s ", t->comm);

		/* Is this the "main" thread of a multithreaded process?
		 * We check by seeing if
		 *  (a) it's a userspace thread, and
		 *  (b) it's TGID == it's PID, and
		 *  (c) there are >1 threads in the process.
		 * If so, display the number of threads in the overall process
		 * to the right..
		 */
		nr_thrds = get_nr_threads(p);
		if (p->mm && (p->tgid == t->pid) && (nr_thrds > 1))
			snprintf(tmp3, TMPMAX - 1, " %3d", nr_thrds);

		pr_info("%s%s%s\n", tmp1, tmp2, tmp3);

		total++;
		memset(tmp1, 0, sizeof(tmp1));
		memset(tmp2, 0, sizeof(tmp2));
		memset(tmp3, 0, sizeof(tmp3));

		put_task_struct(t);	/* release reference to the task struct */
	} while_each_thread(p, t);
	rcu_read_unlock();

	return total;
}

static int __init thrd_showall_init(void)
{
	int total;

	pr_info("%s: inserted\n", KBUILD_MODNAME);
	total = showthrds();
	pr_info("%s: total # of threads on the system: %d\n", KBUILD_MODNAME, total);

	return 0;		/* success */
}
static void __exit thrd_showall_exit(void)
{
	pr_info("%s: removed\n", KBUILD_MODNAME);
}
module_init(thrd_showall_init);
module_exit(thrd_showall_exit);
