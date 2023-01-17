/*
 * ch7/kernel_seg/kernel_seg.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 7: Kernel and Memory Management Internals Essentials
 ****************************************************************
 * Brief Description:
 * A kernel module to show us some relevant details wrt the layout of the
 * kernel segment, IOW, the kernel VAS (Virtual Address Space). In effect,
 * this shows a simple memory map of the kernel. Works on both 32 and 64-bit
 * systems of differing architectures (note: only lightly tested on Aarch32,
 * Aarch64, x86-32 and x86_64 systems).
 * Optional: displays key info of the user VAS if the module parameter
 * show_uservas is set to 1.
 *
 * Useful! With show_uservas=1 we literally 'see' the full memory map of the
 * process, including kernel-space.
 * (Also, fyi, for a more detailed view of the kernel/user VAS, check out the
 * 'procmap' utility).
 *
 * For details, please refer the book, Ch 7.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <asm/pgtable.h>
#include <asm/fixmap.h>
#include "../../klib_llkd.h"
#include "../../convenient.h"

#define OURMODNAME   "show_kernel_seg"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP book 2E:ch7/kernel_seg: display some kernel segment details");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/* Module parameters */
static int show_uservas;
module_param(show_uservas, int, 0660);
MODULE_PARM_DESC(show_uservas, "Show some user space VAS details; 0 = no (default), 1 = show");

#define ELLPS "|                         [ . . . ]                           |\n"

extern void llkd_minsysinfo(void);	// it's in our klib_llkd 'library'

/*
 * show_userspace_info
 * Display some arch-independent details of the usermode VAS.
 * Format (for most of the details):
 *  |<name of region>:   start_addr - end_addr        | [ size in KB/MB/GB]
 *
 * f.e. on an x86_64 VM with ~ 2981 MB RAM
 *  |       text segment  000055dca340b000 - 000055dca3425285     | [       104 KB]
 *
 * We order it by descending address (here, uva's).
 */
static void show_userspace_info(void)
{
	pr_info("+------- Above this line: kernel VAS; below: user VAS --------+\n"
		ELLPS
		"|Process environment "
#if (BITS_PER_LONG == 64)
		" %px - %px     | [ %4zu bytes]\n"
		"|          arguments "
		" %px - %px     | [ %4zu bytes]\n"
		"|        stack start  %px                        |\n"
		"|       heap segment "
		" %px - %px     | [ %9zu KB]\n"
		"|static data segment "
		" %px - %px     | [ %4zu bytes]\n"
		"|       text segment "
		" %px - %px     | [ %9zu KB]\n"
#else // 32-bit
		" %px - %px                     | [ %4zu bytes]\n"
		"|          arguments "
		" %px - %px                     | [ %4zu bytes]\n"
		"|        stack start  %px                                |\n"
		"|       heap segment "
		" %px - %px                     | [ %9zu KB]\n"
		"|static data segment "
		" %px - %px                     | [ %4zu bytes]\n"
		"|       text segment "
		" %px - %px                     | [ %9zu KB]\n"
#endif
		ELLPS
		"+-------------------------------------------------------------+\n",
		SHOW_DELTA_b((void *)current->mm->env_start, (void *)current->mm->env_end),
		SHOW_DELTA_b((void *)current->mm->arg_start, (void *)current->mm->arg_end),
		(void *)current->mm->start_stack,
		SHOW_DELTA_K((void *)current->mm->start_brk, (void *)current->mm->brk),
		SHOW_DELTA_b((void *)current->mm->start_data, (void *)current->mm->end_data),
		SHOW_DELTA_K((void *)current->mm->start_code, (void *)current->mm->end_code)
	    );

	pr_info(
#if (BITS_PER_LONG == 64)
		       "Kernel and User VAS size (TASK_SIZE) each = %zu bytes [  %zu GB]\n"
#else	// 32-bit
		       "Size of User VAS size (TASK_SIZE) = %10lu bytes            [  %lu GB]\n"
#endif
		       " # userspace memory regions (VMAs) = %d\n",
#if (BITS_PER_LONG == 64)
				TASK_SIZE, (TASK_SIZE >> 30),
#else	// 32-bit
				TASK_SIZE, (TASK_SIZE >> 20),
#endif
		       current->mm->map_count);

#ifdef DEBUG
	pr_info("[DEBUG] Above statistics are wrt 'current' thread (see below):\n");
	PRINT_CTX();		/* show which process is the one in context */
#endif
}

/*
 * show_kernelseg_info
 * Display kernel segment details as applicable to the architecture we're
 * currently running upon.
 * Format (for most of the details):
 *  |<name of region>:   start_addr - end_addr        | [ size in KB/MB/GB/TB]
 *
 * f.e. on an x86_64 VM with ~ 2981 MB RAM
 *  |lowmem region:       ffff956dc0000000 - ffff956e7a58d000     | [     2981 MB]
 *
 * We try to order it by descending address (here, kva's) but this doesn't
 * always work out as ordering of regions differs by arch.
 */
static void show_kernelseg_info(void)
{
	unsigned long ram_size;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	ram_size = totalram_pages() * PAGE_SIZE;
#else // totalram_pages() undefined on the BeagleBone running an older 4.19 kernel..
	ram_size = totalram_pages * PAGE_SIZE;
#endif
	pr_info("PAGE_SIZE = %lu, total RAM ~= %lu MB (%lu bytes)\n",
			PAGE_SIZE, ram_size/(1024*1024), ram_size);

#if defined(CONFIG_ARM64)
	pr_info("VA_BITS (CONFIG_ARM64_VA_BITS) = %d\n", VA_BITS);
	if (VA_BITS > 48 && PAGE_SIZE == (64*1024)) // typically 52 bits and 64K pages
		pr_info("*** >= ARMv8.2 with LPA? (YMMV, not supported here) ***\n");
#endif
	pr_info("Some Kernel Details [by decreasing address; values are approximate]\n"
		"+-------------------------------------------------------------+\n");

	/* ARM-32 vector table */
#if defined(CONFIG_ARM)
	/* On ARM, the definition of VECTORS_BASE turns up only in kernels >= 4.11 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 11, 0)
	pr_info(ELLPS
		"|vector table:       "
		" %px - %px                     | [%5zu KB]\n",
		SHOW_DELTA_K((void *)VECTORS_BASE, (void *)(VECTORS_BASE + PAGE_SIZE)));
#endif
#endif

	/* kernel fixmap region */
	pr_info(ELLPS
		"|fixmap region:      "
#if defined(CONFIG_ARM)
		" %px - %px                     | [%5zu MB]\n",
		SHOW_DELTA_M((void *)FIXADDR_START, (void *)FIXADDR_END)
#else
#if defined(CONFIG_ARM64) || defined(CONFIG_X86)
		" %px - %px     | [%9zu MB]\n",
		SHOW_DELTA_M((void *)FIXADDR_START, (void *)(FIXADDR_START + FIXADDR_SIZE))
#endif
#endif
	);

	/* kernel module region
	 * For the modules region, it's high in the kernel segment on typical 64-bit
	 * systems, but the other way around on many 32-bit systems (particularly
	 * ARM-32); so we rearrange the order in which it's shown depending on the
	 * arch, thus trying to maintain a 'by descending address' ordering.
	 */
#if (BITS_PER_LONG == 64)
	pr_info("|module region:      "
		" %px - %px     | [%9zu MB]\n",
		SHOW_DELTA_M((void *)MODULES_VADDR, (void *)MODULES_END));
#endif

#ifdef CONFIG_KASAN		/* KASAN region: Kernel Address SANitizer */
	pr_info("|KASAN shadow:       "
#if (BITS_PER_LONG == 64)
		" %px - %px     | [%9zu MB = %6zu GB ~= %3zu TB]\n",
		SHOW_DELTA_MGT((void *)KASAN_SHADOW_START, (void *)KASAN_SHADOW_END)
#else  // 32-bit w/ KASAN enabled
		" %px - %px                     | [%9zu MB = %6zu GB]\n",
		SHOW_DELTA_MG((void *)KASAN_SHADOW_START, (void *)KASAN_SHADOW_END)
#endif
	);
#endif

	/* sparsemem vmemmap */
#if defined(CONFIG_SPARSEMEM_VMEMMAP) && defined(CONFIG_ARM64) // || defined(CONFIG_X86))
	pr_info(ELLPS
		"|vmemmap region:     "
		" %px - %px     | [%9zu MB = %6zu GB ~= %3zu TB]\n",
		SHOW_DELTA_MGT((void *)VMEMMAP_START, (void *)VMEMMAP_START + VMEMMAP_SIZE));
#endif
#if defined(CONFIG_X86) && (BITS_PER_LONG==64)
	// TODO: no size macro for X86_64?
	pr_info(ELLPS
		"|vmemmap region start %px                        |\n",
		(void *)VMEMMAP_START);
#endif

	/* vmalloc region */
	pr_info("|vmalloc region:     "
#if (BITS_PER_LONG == 64)
		" %px - %px     | [%9zu MB = %6zu GB ~= %3zu TB]\n",
		SHOW_DELTA_MGT((void *)VMALLOC_START, (void *)VMALLOC_END)
#else  // 32-bit
		" %px - %px                     | [%5zu MB]\n",
		SHOW_DELTA_M((void *)VMALLOC_START, (void *)VMALLOC_END)
#endif
	);

	/* lowmem region (RAM direct-mapping) */
/*
	pr_debug(" PO=%lx=%lu; PAGE_OFFSET + ram_size = %lx = %lu\n"
		"0xc0000000 + ram_size(%lu=0x%lx) = 0x%lx\n",
		(unsigned long)(PAGE_OFFSET), (unsigned long)(PAGE_OFFSET),
		(unsigned long)(PAGE_OFFSET) + ram_size,
		(unsigned long)(PAGE_OFFSET) + ram_size,
		ram_size, ram_size, 0xc0000000 + ram_size);
*/
	pr_info("|lowmem region:      "
#if (BITS_PER_LONG == 32)
		" %px - %px                     | [%5zu MB]\n"
		"|                     ^^^^^^^^                                |\n"
		"|                    PAGE_OFFSET                              |\n",
#else
		" %px - %px     | [%9zu MB]\n"
		"|                     ^^^^^^^^^^^^^^^^                        |\n"
		"|                        PAGE_OFFSET                          |\n",
#endif
		SHOW_DELTA_M((void *)PAGE_OFFSET, (void *)(PAGE_OFFSET) + ram_size));

	/* (possible) highmem region;  may be present on some 32-bit systems */
#if defined(CONFIG_HIGHMEM) && (BITS_PER_LONG==32)
	pr_info("|HIGHMEM region:     "
		" %px - %px                     | [%5zu MB]\n",
		SHOW_DELTA_M((void *)PKMAP_BASE, (void *)((PKMAP_BASE) + (LAST_PKMAP * PAGE_SIZE))));
#endif

	/*
	 * Symbols for the kernel image itself:
	 *   text begin/end (_text/_etext)
	 *   init begin/end (__init_begin, __init_end)
	 *   data begin/end (_sdata, _edata)
	 *   bss begin/end (__bss_start, __bss_stop)
	 * are only defined *within* the kernel (in-tree) and aren't available for
	 * modules; thus we don't attempt to print them.
	 */

#if (BITS_PER_LONG == 32)	/* modules region: see the comment above reg this */
	pr_info("|module region:      "
		" %px - %px                     | [%5zu MB]\n",
		SHOW_DELTA_M((void *)MODULES_VADDR, (void *)MODULES_END));
#endif
	pr_info(ELLPS);
}

static int __init kernel_seg_init(void)
{
	pr_info("%s: inserted\n", OURMODNAME);

	/* Display some minimal system info
	 * Note: this function is within our kernel 'library' here:
	 *  ../../llkd_klib.c
	 * Hence, we must arrange to link it in (see the Makefile)
	 */
	llkd_minsysinfo();
	show_kernelseg_info();

	if (show_uservas)
		show_userspace_info();
	else {
		pr_info("+-------------------------------------------------------------+\n");
		pr_info("%s: skipping show userspace...\n", OURMODNAME);
	}

	return 0;		/* success */
}

static void __exit kernel_seg_exit(void)
{
	pr_info("%s: removed\n", OURMODNAME);
}

module_init(kernel_seg_init);
module_exit(kernel_seg_exit);
