/*
 * ch8/slab3_maxsize/slab3_maxsize.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 8 : Linux Kernel Memory Allocation for Module Authors, Part 1
 ****************************************************************
 * Brief Description:
 *
 * For details, please refer the book, Ch 8.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP2E book:ch8/slab3_maxsize: test max alloc limit from k[m|z]alloc()");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int stepsz = 204800;  // 200 Kb
module_param(stepsz, int, 0644);
MODULE_PARM_DESC(stepsz,
"Amount to increase allocation by on each loop iteration (default=200 KB");

static int test_maxallocsz(void)
{
	size_t size2alloc = 0;

	while (1) {
		void *p = kmalloc(size2alloc, GFP_KERNEL);
		if (!p) {
			pr_alert("kmalloc fail, size2alloc=%zu\n", size2alloc);
			// WARN_ONCE(1, "kmalloc fail, size2alloc=%zu\n", size2alloc);
			return -ENOMEM;
		}
		pr_info("kmalloc(%7zu) = 0x%px\n", size2alloc, p);
		kfree(p);
		size2alloc += stepsz;
	}
	return 0;
}

static int __init slab3_maxsize_init(void)
{
	pr_info("%s: inserted\n", KBUILD_MODNAME);
	return test_maxallocsz();
}

static void __exit slab3_maxsize_exit(void)
{
	pr_info("%s: removed\n", KBUILD_MODNAME);
}

module_init(slab3_maxsize_init);
module_exit(slab3_maxsize_exit);
