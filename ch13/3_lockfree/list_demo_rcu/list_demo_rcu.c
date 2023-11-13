/*
 * ch13/list_demo_rcu/list_demo_rcu.c
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
 * *** UPDATED to include RCU synchronization (and a spinlock to protect writers
 * from stepping on each other) ***
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

LIST_HEAD(head_node);
struct node {
	struct list_head list;	/* first member should be this one; it has
				 * the pointers to next and prev
				 */
	int ival1, ival2;
	s8 letter;
};

int add2tail(int v1, int v2, s8 achar, spinlock_t *lock)
{
	struct node *mynode = NULL;

	mynode = kzalloc(sizeof(struct node), GFP_KERNEL);
	if (!mynode)
		return -ENOMEM;
	mynode->ival1 = v1;
	mynode->ival2 = v2;
	mynode->letter = achar;

	INIT_LIST_HEAD(&mynode->list);

	/*--- Update (write) to the data structure in qs; we need to protect
	 * against concurrency, we employ a spinlock
	 */
	pr_info("list update: using spinlock\n");
	spin_lock(lock);
	// void list_add_tail(struct list_head *new, struct list_head *head)
	list_add_tail(&mynode->list, &head_node);
	spin_unlock(lock);
	pr_info("Added a node (with letter '%c') to the list...\n", achar);

	return 0;
}

void showlist(void)
{
/**     struct list_head *ptr; **/
	struct node *curr;

	if (list_empty(&head_node))
		return;

	pr_info("   val1   |   val2   | letter\n");
/**
	list_for_each(ptr, &head_node) {
		curr = list_entry(ptr, struct node, list); // wrapper over container_of()
**/
	// simpler: internally invokes __container_of() to get the ptr to curr struct

	rcu_read_lock();
	list_for_each_entry(curr, &head_node, list) {
		pr_info("%9d %9d   %c\n", curr->ival1, curr->ival2, curr->letter);
	}
	rcu_read_unlock();
}

/* Works, but is O(n) */
void findinlist_letter(s8 char2locate)
{
	struct node *curr;
	int i = 0;
	bool found = false;

	if (list_empty(&head_node))
		return;

	pr_info("Searching list for letter '%c'...\n", char2locate);
	list_for_each_entry(curr, &head_node, list) {
		if (curr->letter == char2locate) {
			found = true;
			pr_info("found '%c' @ node #%d:\n"
				"%9d %9d   _%c_\n",
				char2locate, i, curr->ival1, curr->ival2, curr->letter);
		}
		i++;
	}
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
