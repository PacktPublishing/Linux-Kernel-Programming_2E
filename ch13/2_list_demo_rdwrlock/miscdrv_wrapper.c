/*
 * ch13/3_list_demo_rdwrlock/list_demo_rdwrlock.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 13 : Kernel Synchronization - Part 2
 ****************************************************************
 * Brief Description:
 *
 * This module is based upon our earlier Ch 6 demo for manipulating lists
 * (via the kernel's builtin list.h routines): ch6/list_demo.
 * There, it was unprotected from concurrent access (which is just wrong).
 * So here, we protect against concurrent access by employing the reader-writer
 * spinlock.
 *
 * (This driver is built upon the LKP Part 2 book's first chapter 'misc' driver here:
 * https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2/tree/main/ch1/miscdrv_rdwr ).
 *
 * For details, please refer Ch 13 of the book.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>         // k[m|z]alloc(), k[z]free(), ...
#include <linux/mm.h>           // kvmalloc()
#include <linux/fs.h>		// the fops structure

// copy_[to|from]_user()
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 11, 0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#include <linux/rwlock.h>	// reader-writer spinlock
//#include "../../convenient.h"

/* Function prototypes of funcs in the list.c file */
int add2tail(int v1, int v2, s8 achar, rwlock_t *rwlock);
void showlist(rwlock_t *rwlock);
void freelist(rwlock_t *rwlock);

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION(
"LKP2E book:ch13/2_list_demo_rdwrlock: simple misc char driver rewritten with list usage protected via a reader-writer spinlock");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int buggy;
module_param(buggy, int, 0600);
MODULE_PARM_DESC(buggy,
"If 1, cause an error by issuing a blocking call within an reader-writer spinlock critical section");

DEFINE_RWLOCK(rwlock); /* this reader-writer spinlock protects the list */

/*--- The driver 'methods' follow ---*/
/*
 * open_miscdrv_rdwr()
 * The driver's open 'method'; this 'hook' will get invoked by the kernel VFS
 * when the device file is opened. Here, we simply print out some relevant info.
 * The POSIX standard requires open() to return the file descriptor in success;
 * note, though, that this is done within the kernel VFS (when we return). So,
 * all we do here is return 0 indicating success.
 */
static int open_miscdrv_rdwr(struct inode *inode, struct file *filp)
{
	//PRINT_CTX();		// displays process (or intr) context info
	return 0;
}

/*
 * read_miscdrv_rdwr()
 * The driver's read 'method'; it has effectively 'taken over' the read syscall
 * functionality!
 * The POSIX standard requires that the read() and write() system calls return
 * the number of bytes read or written on success, 0 on EOF and -1 (-ve errno)
 * on failure; here, we display (read) the nodes on the list.
 */
static ssize_t read_miscdrv_rdwr(struct file *filp, char __user *ubuf,
				size_t count, loff_t *off)
{
//	PRINT_CTX();
	pr_info("%s wants to read (upto) %zu bytes\n", current->comm, count);
	showlist(&rwlock);

	return count;
}

/*
 * write_miscdrv_rdwr()
 * The driver's write 'method'; it has effectively 'taken over' the write syscall
 * functionality!
 * The POSIX standard requires that the read() and write() system calls return
 * the number of bytes read or written on success, 0 on EOF and -1 (-ve errno)
 * on failure; Here, we accept the string passed to us and update our 'secret'
 * value to it.
 */
static ssize_t write_miscdrv_rdwr(struct file *filp, const char __user *ubuf,
				size_t count, loff_t *off)
{
//	PRINT_CTX();

	/* Add a few nodes to the tail of the list */
	add2tail(1, 2, 'R', &rwlock);
	add2tail(3, 1415, 'C', &rwlock);
	add2tail(jiffies, jiffies+msecs_to_jiffies(300), 'U', &rwlock);

	return count;
}

/*
 * close_miscdrv_rdwr()
 * The driver's close 'method'; this 'hook' will get invoked by the kernel VFS
 * when the device file is closed (technically, when the file ref count drops
 * to 0). Here, we simply print out some info, and return 0 indicating success.
 */
static int close_miscdrv_rdwr(struct inode *inode, struct file *filp)
{
	//PRINT_CTX();		// displays process (or intr) context info
	return 0;
}

/* The driver 'functionality' is encoded via the fops */
static const struct file_operations lkp_misc_fops = {
	.open = open_miscdrv_rdwr,
	.read = read_miscdrv_rdwr,
	.write = write_miscdrv_rdwr,
	.llseek = no_llseek,    // dummy, we don't support lseek(2)
	.release = close_miscdrv_rdwr,
	/* As you learn more reg device drivers (refer this book's companion guide
	 * 'Linux Kernel Programming (Part 2): Writing character device drivers: Learn
	 * to work with user-kernel interfaces, handle peripheral I/O & hardware
	 * interrupts '), you'll realize that the ioctl() would be a very useful method
	 * here. As an exercise, implement an ioctl method; when issued with the
	 * 'GETSTATS' 'command', it should return the statistics (tx, rx, errors) to
	 * the calling app.
	 */
};

static struct miscdevice lkp_miscdev = {
	.minor = MISC_DYNAMIC_MINOR, // kernel dynamically assigns a free minor#
	.name = "lkp_miscdrv_list_rdwrlock",
	    // populated within /sys/class/misc/ and /sys/devices/virtual/misc/
	.mode = 0666,       /* ... dev node perms set as specified here */
	.fops = &lkp_misc_fops,     // connect to 'functionality'
};

static int __init list_demo_rdwrlock_init(void)
{
	int ret;

	ret = misc_register(&lkp_miscdev);
	if (ret < 0) {
		pr_notice("misc device registration failed, aborting\n");
		return ret;
	}
	pr_info("LKP misc driver for rwlock demo (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", lkp_miscdev.minor, lkp_miscdev.name);

	return 0;		/* success */
}

static void __exit list_demo_rdwrlock_exit(void)
{
	freelist(&rwlock);
	misc_deregister(&lkp_miscdev);
	pr_info("LKP misc driver %s for rwlock demo deregistered, bye\n", lkp_miscdev.name);
}

module_init(list_demo_rdwrlock_init);
module_exit(list_demo_rdwrlock_exit);
