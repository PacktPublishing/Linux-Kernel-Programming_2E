/*
 * ch7/sparsemem_show/sparsemem_show.c
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
 * This kernel module retrieves some minimal info wrt the kernel physical
 * memory model- the 'sparsemem-vmemmap' one.
 *
 * For details, please refer the book, Ch 7.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
    ("LKP2E:ch7/sparsemem_show: display minimal info reg the kernel physical memory model");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int __init sparsemem_show_init(void)
{
	pr_info("inserted\n");
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	pr_info("VMEMMAP_START = 0x%016lx\n"
		"SECTION_SIZE_BITS = %u, so size of each section=2^%u = %u bytes = %u MB\n"
		"max # of sections=%lu\n"
		"MAX_PHYSMEM_BITS=%u; so max supported physical addr space (RAM): %lu GB = %lu TB\n",
		VMEMMAP_START,
		SECTION_SIZE_BITS, SECTION_SIZE_BITS, (1 << SECTION_SIZE_BITS),
		(1 << SECTION_SIZE_BITS) >> 20, NR_MEM_SECTIONS, MAX_PHYSMEM_BITS,
		(1UL << MAX_PHYSMEM_BITS) >> 30, (1UL << MAX_PHYSMEM_BITS) >> 40);
#else
	pr_info("SPARSEMEM_VMEMMAP not supported\n");
#endif
	return 0;		/* success */
}

static void __exit sparsemem_show_exit(void)
{
	pr_info("removed\n");
}

module_init(sparsemem_show_init);
module_exit(sparsemem_show_exit);
