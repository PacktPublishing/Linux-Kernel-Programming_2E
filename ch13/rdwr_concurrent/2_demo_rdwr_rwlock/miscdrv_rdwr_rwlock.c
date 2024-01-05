/*
 * ch13/rdwr_concurrent/1_demo_rdwr_nolocks/miscdrv_rdwr_rwlock.c
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
 * This driver is built upon the LKP Part 2 book's first chapter 'misc' driver here:
 * https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2/tree/main/ch1/miscdrv_rdwr
 *
 * This code is 'Case 2: Use the reader-writer spinlock' of a three case demo,
 * showing some ways to use locking/sync constructs in a read-mostly scenario.
 * (It's like this:
 *      Case 1                 Case 2                  Case 3
 * No locks; just wrong    Use the read-writer      Use RCU sync!
 *                          spinlock                   Best.
 *                         ^^^^^^^^^^^^^^^^^^^^
 *                            <this code demo>
 * ).
 *
 * The demo has reader and writer threads running, concurrently reading
 * and writing a global data structure, IOW, shared state.
 * This of course constitutes a critical section(s); if we leave it be,
 * we'll end up with data races, data corruption.
 * So, here, we protect the critical sections using a reader-writer spinlock.
 *
 * For details, please refer both books, LKP 2E Ch 13 and LKP-Part 2, Ch 1.
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
#include "../../../convenient.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION(
"LKP2E book:ch13/rdwr_concurrent/miscdrv_rdwr_rwlock: simple misc char driver demo: concurrent reads/writes protected via a reader-writer spinlock");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int buggy;
module_param(buggy, int, 0600);
MODULE_PARM_DESC(buggy,
"If 1, cause an error by issuing a blocking call within an reader-writer spinlock critical section");

#define	SHOW_DATA() do {   \
 pr_info("gd: gps_lock=%d, (lat,long,alt)=(%lu,%lu,%lu), issue_in_l6=%d\n", \
	gd->gps_lock, gd->lat, gd->longit, gd->alt, gd->issue_in_l6);       \
} while(0)

static struct global_data {
	bool gps_lock;
	long lat, longit, alt;
	int issue_in_l6;
	rwlock_t rwlock;
} *gd;

static int reader(struct global_data *gd)
{
	long x, y, z;
	int stat;

	read_lock(&gd->rwlock);
	stat = gd->issue_in_l6;
	if (gd->gps_lock) {
		x = gd->lat;
		y = gd->longit;
		z = gd->alt;
	}
	read_unlock(&gd->rwlock);

	return stat;
}

static void writer(struct global_data *gd)
{
	long x = 177564, y = 773540, z = 920;

	write_lock(&gd->rwlock);
	gd->lat = x;
	gd->longit = y;
	gd->alt = z;
	gd->issue_in_l6 = 1;
	write_unlock(&gd->rwlock);
}


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
	//PRINT_CTX();
	//pr_info("%s wants to read (upto) %zu bytes\n", current->comm, count);
	reader(gd);
	SHOW_DATA();

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
	PRINT_CTX();
	writer(gd);

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
	.name = "lkp_miscdrv_rdwr_rwlock",
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
	 gd = kzalloc(sizeof(struct global_data), GFP_KERNEL);
         if (!gd)
                 return -ENOMEM;
         gd->gps_lock = 1;

	pr_info("LKP misc driver for rdwr with rw-lock demo (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", lkp_miscdev.minor, lkp_miscdev.name);
	rwlock_init(&gd->rwlock); // init the rw-lock to the unlocked state

	return 0;		/* success */
}

static void __exit list_demo_rdwrlock_exit(void)
{
	kfree(gd);
	misc_deregister(&lkp_miscdev);
	pr_info("LKP misc driver %s for rdwr with rw-lock demo deregistered, bye\n", lkp_miscdev.name);
}

module_init(list_demo_rdwrlock_init);
module_exit(list_demo_rdwrlock_exit);
