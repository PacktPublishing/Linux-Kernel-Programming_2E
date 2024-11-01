/*
 * ch5/min_sysinfo/min_sysinfo.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 5: Writing your First Kernel Module - LKMs Part 2
 ****************************************************************
 * Brief Description:
 * Make use of some convenience macros provided by the kernel to glean and
 * print some minimal CPU, bit-width, endian-ness details.
 * Can be cross-compiled and tried out on various Linux systems.
 *
 * For details, please refer the book, Ch 5.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/limits.h>
#include "../../convenient.h"

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("LKP2E book:ch5/min_sysinfo: print some minimal system info");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.2");

int my_snprintf_lkp(char *buf, size_t maxsize, const char *fmt, ...);
void show_sizeof(void);
void llkd_sysinfo2(void);
void llkd_sysinfo(void);

/*
 * Simple wrapper over the snprintf() - a bit more security-aware.
 * Checks for and warns on overflow.
 * (Normally, you'd imagine a 'library' routine like this would be defined in
 * some sort of 'library' source; we do exactly this, when we reach that point
 * in the book! So, pedantically, here we simply define it with a 'my_' prefix
 * within this file itself...
 */
int my_snprintf_lkp(char *buf, size_t maxsize, const char *fmt, ...)
{
	va_list args;
	int n;

#ifndef __KERNEL__
#include <stdarg.h>
#endif
	va_start(args, fmt);
	n = vsnprintf(buf, maxsize, fmt, args);
	va_end(args);
	if (n >= maxsize) {
#ifdef __KERNEL__
		pr_warn("snprintf(): possible overflow! (maxsize=%lu, ret=%d)\n",
			maxsize, n);
		dump_stack();
#else
		fprintf(stderr, "snprintf(): possible overflow! (maxsize=%lu, ret=%d)\n",
			maxsize, n);
#endif
	}
	return n;
}

/*
 * show_sizeof()
 * Simply displays the sizeof data types on the platform.
 */
void show_sizeof(void)
{
	pr_info("sizeof: (bytes)\n"
		"  char = %2zu   short int = %2zu              int = %2zu\n"
		"  long = %2zu   long long = %2zu           void * = %2zu\n"
		" float = %2zu      double = %2zu      long double = %2zu\n",
		sizeof(char), sizeof(short int),
		sizeof(int), sizeof(long), sizeof(long long), sizeof(void *),
		sizeof(float), sizeof(double), sizeof(long double));
}

/* llkd_sysinfo2:
 * A more security-aware version of the llkd_sysinfo routine. We used
 * David Wheeler's flawfinder(1) tool to detect possible vulnerabilities:
 * so, we change the strlen, and replace the strncat with strlcat.
 */
void llkd_sysinfo2(void)
{
#define MSGLEN   128
	char msg[MSGLEN];

	memset(msg, 0, MSGLEN);
	my_snprintf_lkp(msg, 48, "%s(): minimal Platform Info:\nCPU: ", __func__);

	/* Strictly speaking, all this #if...#endif is considered ugly and should be
	 * isolated as far as is possible
	 */
#ifdef CONFIG_X86
#if (BITS_PER_LONG == 32)
	strlcat(msg, "x86_32, ", MSGLEN);
#else
	strlcat(msg, "x86_64, ", MSGLEN);
#endif
#endif
#ifdef CONFIG_ARM
	strlcat(msg, "AArch32 (ARM-32), ", MSGLEN);
#endif
#ifdef CONFIG_ARM64
	strlcat(msg, "AArch64 (ARM-64), ", MSGLEN);
#endif
#ifdef CONFIG_MIPS
	strlcat(msg, "MIPS, ", MSGLEN);
#endif

#ifdef CONFIG_PPC
	strlcat(msg, "PowerPC, ", MSGLEN);
#endif
#ifdef CONFIG_S390
	strlcat(msg, "IBM S390, ", MSGLEN);
#endif

#ifdef __BIG_ENDIAN
	strlcat(msg, "big-endian; ", MSGLEN);
#else
	strlcat(msg, "little-endian; ", MSGLEN);
#endif

#if (BITS_PER_LONG == 32)
	strlcat(msg, "32-bit OS.\n", MSGLEN);
#elif(BITS_PER_LONG == 64)
	strlcat(msg, "64-bit OS.\n", MSGLEN);
#endif
	pr_info("%s", msg);

	show_sizeof();

	/* Word ranges: min & max: defines are in include/linux/limits.h */
	pr_info
	    ("Word [U|S][8|16|32|64] ranges: unsigned max, signed max, signed min:\n"
	     " U8_MAX = %20u = 0x%16x,  S8_MAX = %20d = 0x%16x,  S8_MIN = %20d = 0x%16x\n"
	     "U16_MAX = %20u = 0x%16x, S16_MAX = %20d = 0x%16x, S16_MIN = %20d = 0x%16x\n"
	     "U32_MAX = %20u = 0x%16x, S32_MAX = %20d = 0x%16x, S32_MIN = %20d = 0x%16x\n"
	     "U64_MAX = %20llu = 0x%16llx, S64_MAX = %20lld = 0x%16llx, S64_MIN = %20lld = 0x%16llx\n"
	     /* PHYS_ADDR_MAX is a mask of all address bits set to 1 (32 or 64
	      * depending on the processor
		  */
#if (BITS_PER_LONG == 32)
	      "PHYS_ADDR_MAX = %u = 0x%px\n"
#else
	      "PHYS_ADDR_MAX = %llu = 0x%px\n"
#endif
	     , U8_MAX, U8_MAX, S8_MAX, S8_MAX, S8_MIN, S8_MIN,
	     U16_MAX, U16_MAX, S16_MAX, S16_MAX, S16_MIN, S16_MIN,
	     U32_MAX, U32_MAX, S32_MAX, S32_MAX, S32_MIN, S32_MIN,
	     U64_MAX, U64_MAX, S64_MAX, S64_MAX, S64_MIN, S64_MIN
	     , PHYS_ADDR_MAX, (void *)PHYS_ADDR_MAX);
}
EXPORT_SYMBOL(llkd_sysinfo2);

void llkd_sysinfo(void)
{
	char msg[128];

	memset(msg, 0, 128);
	my_snprintf_lkp(msg, 47, "%s(): minimal Platform Info:\nCPU: ", __func__);

	/* Strictly speaking, all this #if...#endif is considered ugly and should be
	 * isolated as far as is possible
	 */
#ifdef CONFIG_X86
#if (BITS_PER_LONG == 32)
	strncat(msg, "x86_32, ", 9);
#else
	strncat(msg, "x86_64, ", 9);
#endif
#endif
#ifdef CONFIG_ARM
	strncat(msg, "AArch32 (ARM-32), ", 19);
#endif
#ifdef CONFIG_ARM64
	strncat(msg, "AArch64 (ARM-64), ", 19);
#endif
#ifdef CONFIG_MIPS
	strncat(msg, "MIPS, ", 7);
#endif
#ifdef CONFIG_PPC
	strncat(msg, "PowerPC, ", 9);
#endif
#ifdef CONFIG_S390
	strncat(msg, "IBM S390, ", 10);
#endif

#ifdef __BIG_ENDIAN
	strncat(msg, "big-endian; ", 13);
#else
	strncat(msg, "little-endian; ", 16);
#endif

#if (BITS_PER_LONG == 32)
	strncat(msg, "32-bit OS.\n", 12);
#elif(BITS_PER_LONG == 64)
	strncat(msg, "64-bit OS.\n", 12);
#endif
	pr_info("%s", msg);
}
EXPORT_SYMBOL(llkd_sysinfo);

static int __init min_sysinfo_init(void)
{
	pr_info("inserted\n");
	llkd_sysinfo();
	llkd_sysinfo2();
	return 0;		/* success */
}

static void __exit min_sysinfo_exit(void)
{
	pr_info("removed\n");
}

module_init(min_sysinfo_init);
module_exit(min_sysinfo_exit);
