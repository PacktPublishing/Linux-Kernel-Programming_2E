/*
 * ch8/slab1/slab1.c
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
 *
 * For details, please refer the book, Ch 8.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP2E:ch8/slab1: k[m|z]alloc, kfree, basic demo");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

static char *gkptr;
struct myctx {
	u32 iarr[100];
	u64 uarr[100];
	char uname[128], passwd[16], config[16];
};
static struct myctx *ctx;

static int __init slab1_init(void)
{
	/* 1. Allocate slab memory for 1 KB using the kmalloc() */
	gkptr = kmalloc(1024, GFP_KERNEL);
	if (!gkptr) {
		WARN_ONCE(1, "kmalloc() failed!\n");
		/* As mentioned earlier, there is really no need to print an
		 * error msg when a memory alloc fails; the situation
		 * "shouldn't" typically occur, and if it does, the kernel will
		 * emit a chain of messages in any case. Here, we use the WARN_ONCE()
		 * macro pedantically, and as this is a 'learning' program.. In practice,
		 * simply returning an appropriate -ve errno (-ENOMEM here) is enough.
		 */
		goto out_fail1;
	}
	/* We use the %px format specifier here to show the actual KVA; in production, Don't! */
	pr_info("kmalloc() succeeds, (actual KVA) ret value = %px\n", gkptr);

	/* Commit 091cb09 (5.4 kernel) has the print_hex_dump_bytes() perform
	 * printing at log level KERN_DEBUG; in effect, it only shows up if -DDEBUG
	 * (or you use the dynamic debug facility). So, I've set DEBUG on in the Makefile
	 */
	print_hex_dump_bytes("gkptr before memset: ", DUMP_PREFIX_OFFSET, gkptr, 32);
	memset(gkptr, 'm', 1024);
	print_hex_dump_bytes(" gkptr after memset: ", DUMP_PREFIX_OFFSET, gkptr, 32);

	/* 2. Allocate memory for and initialize our 'context' structure */
	ctx = kzalloc(sizeof(struct myctx), GFP_KERNEL);
	if (!ctx)
		goto out_fail2;
	pr_info("context struct alloc'ed and initialized (actual KVA ret = %px)\n", ctx);
	print_hex_dump_bytes("ctx: ", DUMP_PREFIX_OFFSET, ctx, 32);

	return 0;		/* success */

 out_fail2:
	kfree(gkptr);
 out_fail1:
	return -ENOMEM;
}

static void __exit slab1_exit(void)
{
	kfree(ctx);
	kfree(gkptr);
	pr_info("freed slab memory, removed\n");
}

module_init(slab1_init);
module_exit(slab1_exit);
