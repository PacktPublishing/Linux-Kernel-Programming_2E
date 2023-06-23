/*
 * ch6/current_affairs/current_affairs.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 6: Kernel and Memory Management Internals -Essentials
 ****************************************************************
 * Brief Description:
 * A simple module to display a few members of the task structure of the
 * currently running process context! This helps 'prove', in effect, that
 * the Linux kernel follows a monolithic architecture.
 *
 * For details, please refer the book, Ch 6.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>    /* current() */
#include <linux/preempt.h>  /* in_task() */
#include <linux/cred.h>     /* current_{e}{u,g}id() */
#include <linux/uidgid.h>   /* {from,make}_kuid() */

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP book:ch6/current_affairs: display a few members of \
the current process' task structure");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static inline void show_ctx(void)
{
    /* Extract the task UID and EUID using helper methods provided */
    unsigned int uid = from_kuid(&init_user_ns, current_uid());
    unsigned int euid = from_kuid(&init_user_ns, current_euid());

    pr_info("\n");      /* shows mod & func names (due to the pr_fmt()!) */
    if (likely(in_task())) {
        pr_info("we're running in process context ::\n"
            " name        : %s\n"
            " PID         : %6d\n"
            " TGID        : %6d\n"
            " UID         : %6u\n"
            " EUID        : %6u (%s root)\n"
            " state       : %c\n"
            " current (ptr to our process context's task_struct) :\n"
            "               0x%pK (0x%px)\n"
            " stack start : 0x%pK (0x%px)\n",
            current->comm,
            /* always better to use the helper methods provided */
            task_pid_nr(current), task_tgid_nr(current),
            /* ... rather than using direct lookups:
             * current->pid, current->tgid,
             */
            uid, euid,
            (euid == 0 ? "have" : "don't have"),
            task_state_to_char(current),
			/* Printing addresses twice- via %pK and %px
			 * Here, by default, the values will typically be the same as
			 * kptr_restrict == 1 and we've got root.
			 */
            current, current,
			current->stack, current->stack);

            if (task_state_to_char(current) == 'R')
                pr_info("on virtual CPU? %s\n", (current->flags & PF_VCPU)?"yes":"no");
    } else
        pr_alert("Whoa! running in interrupt context [Should NOT Happen here!]\n");
}

static int __init current_affairs_init(void)
{
    pr_info("inserted\n");
    pr_info("sizeof(struct task_struct)=%zd\n", sizeof(struct task_struct));
    show_ctx();
    return 0;       /* success */
}
static void __exit current_affairs_exit(void)
{
    show_ctx();
    pr_info("removed\n");
}
module_init(current_affairs_init);
module_exit(current_affairs_exit);
