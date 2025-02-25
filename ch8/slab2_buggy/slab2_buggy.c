/*
 * ch8/slab2_buggy/slab2_buggy.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 8 : Kernel Memory Allocation for Module Authors, Part 1
 ****************************************************************
 * Brief Description:
 * A quick demo to show the danger of making assumptions!
 * Here, we assume that kfree(ptr) sets 'ptr' to NULL; this simply isn't the
 * case, resulting in a kernel BUG.
 *
 * For details, please refer the book, Ch 8.
 * (This code was initially missing and added later to the book GitHub repo)
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP2E:ch8/slab1: k[m|z]alloc, kfree, basic demo");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int __init slab2_buggy_init(void)
{
	int num = 1024, x = 3;
	static char *kptr; // static; is initialized to NULL

	while (x > 0) {
		if (!kptr) {
			kptr = kzalloc(num, GFP_KERNEL);
			// [... work on the slab memory ...]
			memset(kptr, x, num);
		}
		kfree(kptr);
		/* Here, we're *erronously* assuming that kptr gets set to
		 * NULL! So, as mentioned on page 403 of the LKP 2E book:
		 * "This is definitely not the case (it would have been quite
		 * a nice semantic to have though; also, the same argument
		 * applies to the “usual” user space library APIs). Thus, here,
		 * we hit a dangerous bug: on the loop’s second iteration, the
		 * if condition will likely turn out to be false, thus skipping
		 * the allocation. Then, we hit the kfree() , which, of course,
		 * will now corrupt memory (due to a double-free bug)!"
		 * This will cause a kernel bug!
		 */
		x--;
	}

	return 0;		/* success */
}

static void __exit slab2_buggy_exit(void)
{
	pr_info("removed\n");
}

module_init(slab2_buggy_init);
module_exit(slab2_buggy_exit);
