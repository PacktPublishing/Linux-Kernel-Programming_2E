/*
 * ch13/rdwr_concurrent/3_demo_rdwr_rcu/miscdrv_rdwr_rcu.c
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
 * This code is 'Case 3: Use RCU' of a three case demo, showing some ways to
 * use locking/sync constructs in a read-mostly scenario.
 * (It's like this:
 *      Case 1                 Case 2                  Case 3
 * No locks; just wrong    Use the read-writer     Use RCU sync!
 *                          spinlock                  Best.
 *                                                 ^^^^^^^^^^^^^^
 *                                                <this code demo>
 *
 * The demo has reader and writer threads running, concurrently reading
 * and writing a global data structure, IOW, shared state.
 * This of course constitutes a critical section(s); if we leave it be,
 * we'll end up with data races, data corruption.
 * So, here, we protect via RCU as follows:
 * - Protect the RCU read-side critical section via RCU 'locking'
 * - Protect the write-side critical section via a simple spinlock.
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

#include <linux/spinlock.h>
#include "../../../convenient.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION(
"LKP2E book:ch13/rdwr_concurrent/miscdrv_rdwr_rcu: simple misc char driver demo: concurrent reads/writes protected via RCU");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int buggy;
module_param(buggy, int, 0600);
MODULE_PARM_DESC(buggy,
"If 1, cause an error by issuing a blocking call within an RCU read-side critical section");

#define	SHOW_DATA(gd) do {   \
 pr_info("gd: gps_lock=%d, (lat,long,alt)=(%lu,%lu,%lu), issue_in_l6=%d\n", \
	gd->gps_lock, gd->lat, gd->longit, gd->alt, gd->issue_in_l6);       \
} while(0)

static struct global_data {
	int gps_lock;
	long lat, longit, alt;
	int issue_in_l6;
} *gdata;
static spinlock_t gdata_lock; /* spinlock to protect writers.
	* Why not keep it inside the structure?
	* It's a bit subtle: we need the spinlock during the write critical section;
	* however, it's in here that we create a copy of the data object and
	* modify/update it.
	* If the copy includes the spinlock (which it will) it won't work, as
	* we then violate the contract, using different locks...
	* Trying this, in fact, creates an interesting bug!
	* ...
	* pvqspinlock: lock 0xffff9e... has corrupted value 0x0!
	* ...
	* (The pvqspinlock is a paravirt one (as I ran it on a guest)).
	*/
/* There is no 'read' RCU lock object; readers are meant to run
 * concurrently with each other *and* with writers. It's a 'socially
 * engineered' contract with the developer(s), is all it is!
 */

static int reader(void)
{
	struct global_data *p;
	long x, y, z;
	int stat;

	/* The RCU read-side critical section spans from t1 to t2;
	 * reads run concurrently with both other readers and writers!
	 */
	rcu_read_lock();         // ---t1

	p = rcu_dereference(gdata); /* safely fetch an RCU protected pointer which
			 	     * can then be dereferenced (and used) */
	stat = p->issue_in_l6;
	if (p->gps_lock) {
		x = p->lat;
		y = p->longit;
		z = p->alt;
	}
	rcu_read_unlock();        // ---t2

	return stat;
}
static int writer(void)
{
	struct global_data *gd, *gd_new;
	long x = 129780, y = 775952, z = 920;

	/* The write-side critical section spans from t1 to t2; writes run exclusively */
	spin_lock(&gdata_lock);	//--- t1
	gd = rcu_dereference(gdata); /* safely fetch an RCU protected pointer which
				      * can then be dereferenced (and used) */

	/* The writer creates a copy of the original data object so that it can
	 * work on it while pre-existing RCU readers work on the original
	 */
	gd_new = kzalloc(sizeof(struct global_data), GFP_ATOMIC);
	if (!gd_new) {
		spin_unlock(&gdata_lock);
		return -ENOMEM;
	}

	*gd_new = *gd;	  // copy the content...
	gd_new->lat = x;  // ...and update it as required
	gd_new->longit = y;
	gd_new->alt = z;
	gd_new->issue_in_l6 = 1;

	rcu_assign_pointer(gdata, gd_new);  /* safely and atomically set the
			* new value gd_new on the RCU protected pointer gdata,
			* in effect communicating to (new) readers the change in value
			*/
	spin_unlock(&gdata_lock);	//--- t2

	/* Now have the writer wait (block) for an RCU grace period to elapse,
	 * and then free the just-alloc'ed data object. Waiting this way ensures
	 * that no pre-existing RCU readers remain, that is, they've all finished
	 * their reads, and only then does the writer free the old data object
	 * (and thus sidesteps a potential UAF bug!)
	 */
	synchronize_rcu();
	kfree(gd);

	return 0;
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
	reader();
	SHOW_DATA(gdata);

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
	writer();

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
	.name = "lkp_miscdrv_rdwr_rcu",
	    // populated within /sys/class/misc/ and /sys/devices/virtual/misc/
	.mode = 0666,       /* ... dev node perms set as specified here */
	.fops = &lkp_misc_fops,     // connect to 'functionality'
};

static int __init miscdrv_rdwr_rcu_init(void)
{
	int ret;

	ret = misc_register(&lkp_miscdev);
	if (ret < 0) {
		pr_notice("misc device registration failed, aborting\n");
		return ret;
	}
	gdata = kzalloc(sizeof(struct global_data), GFP_KERNEL);
	if (!gdata)
		return -ENOMEM;
	gdata->gps_lock = 1;
	spin_lock_init(&gdata_lock); // init the spinlock to the unlocked state

	pr_info("LKP misc driver for rdwr with RCU sync demo (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", lkp_miscdev.minor, lkp_miscdev.name);

	return 0;		/* success */
}
static void __exit miscdrv_rdwr_rcu_exit(void)
{
	misc_deregister(&lkp_miscdev);
	pr_info("LKP misc driver %s for rdwr with RCU sync demo deregistered, bye\n", lkp_miscdev.name);
}
module_init(miscdrv_rdwr_rcu_init);
module_exit(miscdrv_rdwr_rcu_exit);
