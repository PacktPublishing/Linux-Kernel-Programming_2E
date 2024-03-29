/*
 * ch6/foreach/prcs_showall.c
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
 * This kernel module iterates over the task structures of all *processes*
 * currently alive on the box, printing out a few details for each of them.
 * We use the for_each_process() macro to do so here.
 *
 * For details, please refer the book, Ch 6.
 */
#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/signal.h>	/* for_each_xxx(), ... */
#endif
#include <linux/fs.h>		/* no_llseek() */
#include <linux/slab.h>
#include <linux/uaccess.h>	/* copy_to_user() */
#include <linux/kallsyms.h>
#include "../../../convenient.h"
#include "../../../klib.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP 2E:ch6/foreach/prcs_showall: Show all processes by iterating over the task list");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int show_prcs_in_tasklist(void)
{
	struct task_struct *p;
#define MAXLEN   128
	char tmp[MAXLEN];
	int numread = 0, total = 0;
	char hdr[] = "     Name       |  TGID  |   PID  |  RUID |  EUID";

	pr_info("%s\n", &hdr[0]);
	/*
	 * (The stuff mentioned on locking here can be skipped on first reading; you can check
	 * it out once you read the materials on kernel synchronization / locking, Ch 12 and
	 * Ch 13; will leave it to you...).
	 * The for_each_process() is a macro that iterates over the task structures in memory.
	 * The task structs are global of course; this implies we should hold a lock of some
	 * sort while working on them (even if only reading!). So, doing
	 *  read_lock(&tasklist_lock);
	 *  [...]
	 *  read_unlock(&tasklist_lock);
	 * BUT, this lock - tasklist_lock - isn't exported and thus unavailable to modules.
	 * So, using an RCU read lock is indicated here.
	 * FYI: a) Ch 12 and Ch 13 cover the details on kernel synchronization.
	 *      b) Read Copy Update (RCU) is a complex synchronization mechanism and implies
	 *         an atomic critical section (no blocking between the RCU lock/unlock).
	 *  RCU's conceptually explained really well in this blog article:
	 *  https://reberhardt.com/blog/2020/11/18/my-first-kernel-module.html
	 */
	rcu_read_lock();
	for_each_process(p) {
		int n = 0;

		memset(tmp, 0, 128);
		get_task_struct(p);	// take a reference to the task struct
		n = snprintf_lkp(tmp, 128, "%-16s|%8d|%8d|%7u|%7u\n", p->comm, p->tgid, p->pid,
			     /* (old way to display credentials): p->uid, p->euid -or-
			      * current_uid().val, current_euid().val
			      * Better way is using the kernel accessor __kuid_val():
			      */
			     __kuid_val(p->cred->uid), __kuid_val(p->cred->euid)
		    );
		put_task_struct(p);	// release reference to the task struct
		numread += n;
		pr_info("%s", tmp);
		//pr_debug("n=%d numread=%d tmp=%s\n", n, numread, tmp);
		//cond_resched(); // don't call this when holding an RCU lock! no blocking APIs...
		total++;
	}
	rcu_read_unlock();

	return total;
}

static int __init prcs_showall_init(void)
{
	int total;

	pr_info("inserted\n");
	total = show_prcs_in_tasklist();
	pr_info("total # of processes on system: %d\n", total);

	return 0;		/* success */
}
static void __exit prcs_showall_exit(void)
{
	pr_info("removed\n");
}
module_init(prcs_showall_init);
module_exit(prcs_showall_exit);
