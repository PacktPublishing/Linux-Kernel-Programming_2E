/*
 * ch8/page_exact_loop/page_exact_loop.c
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
 * A quick demo of using the alloc_pages_exact() and it's counterpart
 * the free_pages_exact() APIs; they're more optimized when allocating
 * larger memory chunks via the page allocator.
 * (But pl read the rest of the chapter and Ch 9 as well!).
 *
 * For details, please refer the book, Ch 8.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
("LKP2E:ch8/page_exact_loop: demo using the superior [alloc|free]_pages_exact() APIs");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

#define MAXTIMES    3 	/* the higher you make this, the more the chance of the
			 * alloc failing, as we only free in the cleanup code path...
			 */
void show_phy_pages(const void *kaddr, size_t len, bool contiguity_check);

static void *gptr[MAXTIMES];
/* Lets ask for 33 KB; the 'regular' PA/BSA APIs will end up giving
 * us 64 KB; the alloc_pages_exact() will optimize it
 */
static size_t gsz = 33*1024;

static int __init page_exact_loop_init(void)
{
	int i, j;

	pr_info("inserted\n");
	for (i = 0; i < MAXTIMES; i++) {
		gptr[i] = alloc_pages_exact(gsz, GFP_KERNEL);
		if (!gptr[i]) {
			// pedantic...
			pr_warn("alloc_pages_exact() failed! (loop index %d)\n", i);
			/* it failed; don't leak, ensure we free the memory taken so far! */
			for (j = i; j >= 0; j--)
				free_pages_exact(gptr[j], gsz);
			return -ENOMEM;
		}
		pr_info("%d: alloc_pages_exact() alloc'ed %zu bytes memory (%zu pages + rem %lu bytes)"
			" from the BSA @ %px\n",
			i, gsz, gsz / PAGE_SIZE, gsz % PAGE_SIZE, gptr[i]);
		// lets 'poison' it..
		memset(gptr[i], 'x', gsz);
		show_phy_pages(gptr[i], gsz, 1);
		msleep(100);
	}
	return 0;		/* success */
}

static void __exit page_exact_loop_exit(void)
{
	int i;

	for (i = 0; i < MAXTIMES; i++)
		free_pages_exact(gptr[i], gsz);
	pr_info("mem freed, removed\n");
}

module_init(page_exact_loop_init);
module_exit(page_exact_loop_exit);
