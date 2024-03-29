/*
 * ch8/slab4_actualsz_wstg_plot/slab4_actualsz_wstg_plot.c
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
 * Here, we have slightly modified the ch8/slab4_actualsize LKM to print just
 * what's required in order to get a good data file, in order to plot a nice
 * graph with gnuplot(1) !
 *
 * For details, please refer the book, Ch 8.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
("LKP2E book:ch8/slab4_actualsz_wstg_plot: test slab alloc with the ksize(), minimal ver");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static int stepsz = 20480; // 20K
module_param(stepsz, int, 0644);
MODULE_PARM_DESC(stepsz,
"Amount to increase allocation by on each loop iteration (default=20K");

static int test_maxallocsz(void)
{
	/* This time, initialize size2alloc to 100 (not 0), as otherwise we'll
	 * likely get a divide error!
	 */
	size_t size2alloc = 100, actual_alloced;
	void *p;

	while (1) {
		p = kmalloc(size2alloc, GFP_KERNEL);
		if (!p) {
			pr_alert("kmalloc fail, size2alloc=%zu\n", size2alloc); // pedantic
			return -ENOMEM;
		}
		actual_alloced = ksize(p);
		/* *Only* print the size2alloc and the percentage of waste */
		pr_info("%zu  %3zu\n",
			size2alloc, (((actual_alloced - size2alloc) * 100) / size2alloc));
		kfree(p);
		size2alloc += stepsz;
	}
	return 0;
}

static int __init slab4_actualsz_wstg_plot_init(void)
{
	pr_info("%s: inserted\n", KBUILD_MODNAME);
	return test_maxallocsz();
}
module_init(slab4_actualsz_wstg_plot_init);
/* Here, we don't require a module cleanup routine, as we know it's
 * going to return failure in the init code path itself and thus we'll
 * never need an rmmod.
 */
