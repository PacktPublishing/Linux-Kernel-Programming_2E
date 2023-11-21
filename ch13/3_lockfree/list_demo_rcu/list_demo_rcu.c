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
 ****************************************************************
 * Brief Description:
 *
 * A simple module to demo the basics of using the kernel's 'famous'
 * linked list macros and routines.
 * Ref: https://www.kernel.org/doc/html/latest/core-api/kernel-api.html#list-management-functions
 *
 * Refactored from the ch13/2_list_demo_rdwrlock module to use RCU instead of
 * the reader-writer spinlock. So, here we employ RCU synchronization (and a
 * spinlock to protect writers).
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
	u64 ival1, ival2;
	s8 letter;
};

int add2tail(u64 v1, u64 v2, s8 achar, spinlock_t *lock)
{
	struct node *mynode = NULL;

	/*
	 * Should we use GFP_KERNEL or GFP_ATOMIC here?
	 * The former if in process context, else the latter.
	 * Yes, BUT we can be in process ctx and still have been called
	 * from an atomic state. So, bottom lne, only the *caller* knows.
	 * Here, let's be safe, and use GFP_ATOMIC.
	 */
	mynode = kzalloc(sizeof(struct node), GFP_ATOMIC);
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
	/* signature: void list_add_tail_rcu(struct list_head *new, struct list_head *head)
	 * '... caller must take whatever precautions are necessary (such as holding
	 * appropriate locks) to avoid racing with another list-mutation primitive...'
	 * Internally uses rcu_assign_pointer() ...
	 */
	list_add_tail_rcu(&mynode->list, &head_node);
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

	pr_info("          val1     |      val2    | letter\n");
/**
	list_for_each_rcu(ptr, &head_node) {
		curr = list_entry_rcu(ptr, struct node, list); // wrapper over container_of()
**/
	/* list_for_each_entry_rcu() is simpler than list_for_each_rcu();
	 * it internally invokes __container_of() to get the ptr to curr struct.
	 * Also, this is the RCU-safe ver: from it's comments:
	 * '... This list-traversal primitive may safely run concurrently with
	 * the _rcu list-mutation primitives such as list_add_rcu()
	 * as long as the traversal is guarded by rcu_read_lock(). ...'
	 */
	rcu_read_lock();
	list_for_each_entry_rcu(curr, &head_node, list) {
		pr_info("%16llu %16llu      %c\n", curr->ival1, curr->ival2, curr->letter);
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
	rcu_read_lock();
	list_for_each_entry_rcu(curr, &head_node, list) {
		if (curr->letter == char2locate) {
			found = true;
			pr_info("found '%c' @ node #%d:\n"
				"%16llu %16llu   _%c_\n",
				char2locate, i, curr->ival1, curr->ival2, curr->letter);
		}
		i++;
	}
	rcu_read_unlock();
	if (!found)
		pr_info("Didn't find '%c' in list\n", char2locate);
}

void freelist(spinlock_t *lock)
{
	struct node *curr;

	if (list_empty(&head_node))
		return;

	pr_info("freeing list items...\n");

	// Wait for any pre-existing RCU readers to cycle off the CPU(s)...
	synchronize_rcu();

	// ... and now delete and free up the list nodes
	spin_lock(lock);
	list_for_each_entry_rcu(curr, &head_node, list) {
		list_del_rcu(&curr->list);
		kfree(curr);
	}
	spin_unlock(lock);
}
