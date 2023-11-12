/*
 * ch13/list_demo_rdwrlock/list_demo_rdwrlock.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * ORIGINALLY from: Ch 6: Kernel Internals Essentials - Processes and Threads
 * Now:
 * Ch 13: Kernel Synchronization - Part 2
 ****************************************************************
 * Brief Description:
 * A simple module to demo the basics of using the kernel's 'famous'
 * linked list macros and routines.
 * Ref: https://www.kernel.org/doc/html/latest/core-api/kernel-api.html#list-management-functions
 *
 * *** UPDATED to include kernel synchronization via the reader-writer
 * spinlock primitives ***
 *
 * For details, please refer the book, Ch 13.
 * License: Dual MIT/GPL
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h> /* the 'famous' header! */
#include <linux/delay.h>
#include "../../convenient.h"

LIST_HEAD(head_node);
struct node {
	struct list_head list;	/* first member should be this one; it has
				 * the pointers to next and prev
				 */
	int ival1, ival2;
	s8 letter;
};

int add2tail(int v1, int v2, s8 achar, rwlock_t *rwlock)
{
	struct node *mynode = NULL;
	u64 t1, t2;

	mynode = kzalloc(sizeof(struct node), GFP_KERNEL);
	if (!mynode)
		return -ENOMEM;
	mynode->ival1 = v1;
	mynode->ival2 = v2;
	mynode->letter = achar;

	INIT_LIST_HEAD(&mynode->list);

	/*--- Update (write) to the data structure in qs; we need to protect
	 * against concurrency, so we use the write (spin)lock passed to us
	 */
	pr_info("list update: using [reader-]writer spinlock\n");
	t1 = ktime_get_real_ns();
	write_lock(rwlock);
	t2 = ktime_get_real_ns();
	// void list_add_tail(struct list_head *new, struct list_head *head)
	list_add_tail(&mynode->list, &head_node);
	write_unlock(rwlock);
	pr_info("Added a node (with letter '%c') to the list...\n", achar);
	pr_info("--- time to get the write lock:\n");
	SHOW_DELTA(t2, t1);

	return 0;
}

void showlist(rwlock_t *rwlock)
{
/**     struct list_head *ptr; **/
	struct node *curr;
	u64 t1, t2;

	if (list_empty(&head_node))
		return;

	pr_info("   val1   |   val2   | letter\n");
/**
	list_for_each(ptr, &head_node) {
		curr = list_entry(ptr, struct node, list); // wrapper over container_of()
**/
	// simpler: internally invokes __container_of() to get the ptr to curr struct

	t1 = ktime_get_real_ns();
	read_lock(rwlock);
	list_for_each_entry(curr, &head_node, list) {
		pr_info("%9d %9d   %c\n", curr->ival1, curr->ival2, curr->letter);
	}
#if 1
	mdelay(750); // deliberately make the reads slower to demo write starvation..
#endif
	read_unlock(rwlock);
	t2 = ktime_get_real_ns();
	pr_info("--- time the read lock was held:\n");
	SHOW_DELTA(t2, t1);
}

/* Works, but is O(n) */
void findinlist_letter(s8 char2locate, rwlock_t *rwlock)
{
	struct node *curr;
	int i = 0;
	bool found = false;

	if (list_empty(&head_node))
		return;

	pr_info("Searching list for letter '%c'...\n", char2locate);
	read_lock(rwlock);
	list_for_each_entry(curr, &head_node, list) {
		if (curr->letter == char2locate) {
			found = true;
			pr_info("found '%c' @ node #%d:\n"
				"%9d %9d   _%c_\n",
				char2locate, i, curr->ival1, curr->ival2, curr->letter);
		}
		i++;
	}
	read_unlock(rwlock);
	if (!found)
		pr_info("Didn't find '%c' in list\n", char2locate);
}

void freelist(void)
{
	struct node *curr;

	if (list_empty(&head_node))
		return;

	pr_info("freeing list items...\n");
	list_for_each_entry(curr, &head_node, list)
	    kfree(curr);
}
