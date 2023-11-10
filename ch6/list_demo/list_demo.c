/*
 * ch6/list_demo/list_demo.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 *
 * From: Ch 6: Kernel Internals Essentials - Processes and Threads
 ****************************************************************
 * Brief Description:
 * A simple module to demo the basics of using the kernel's 'famous'
 * linked list macros and routines.
 * Ref: https://www.kernel.org/doc/html/latest/core-api/kernel-api.html#list-management-functions
 *
 * For details, please refer the book, Ch 6.
 * License: Dual MIT/GPL
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("A simple Linux kernel linked list usage demo");
MODULE_LICENSE("Dual MIT/GPL");	// or whatever
MODULE_VERSION("0.1");

LIST_HEAD(head_node);
struct node {
	struct list_head list; /* first member should be this one; it has
				* the pointers to next and prev
				*/
	int ival1, ival2;
	s8 letter;
};

static int add2tail(int v1, int v2, s8 achar)
{
	struct node *mynode = NULL;

	mynode = kzalloc(sizeof(struct node), GFP_KERNEL);
	if (!mynode)
		return -ENOMEM;
	mynode->ival1 = v1;
	mynode->ival2 = v2;
	mynode->letter = achar;

	INIT_LIST_HEAD(&mynode->list);
	// void list_add_tail(struct list_head *new, struct list_head *head)
	list_add_tail(&mynode->list, &head_node);
	pr_info("Added a node (with letter '%c') to the list...\n", achar);

	return 0;
}

static void showlist(void)
{
	//struct list_head *ptr;
	struct node *curr;

	if (list_empty(&head_node))
		return;

	pr_info("   val1   |   val2   | letter\n");
#if 0
	list_for_each(ptr, &head_node) {
		curr = list_entry(ptr, struct node, list); // wrapper over container_of()
#else
	// simpler: internally invokes __container_of() to get the ptr to curr struct
	list_for_each_entry(curr, &head_node, list) {
#endif
		pr_info("%9d %9d   %c\n",
			curr->ival1, curr->ival2, curr->letter);
	}
}

/* Works, but is O(n) */
static void findinlist_letter(s8 char2locate)
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

static void freelist(void)
{
	struct node *curr;

	if (list_empty(&head_node))
		return;

	pr_info("freeing list items...\n");
	list_for_each_entry(curr, &head_node, list)
		kfree(curr);
}

static int __init list_init(void)
{
#if 0
	struct module mymod;
#endif

	/* Add a few nodes to the tail of the list */
	add2tail(1, 2, 'l');
	add2tail(5, 1000, 'i');
	add2tail(3, 1415, 's');
	add2tail(jiffies, jiffies+msecs_to_jiffies(300), 't');

	// display the list items
	showlist();

	// search for some items in the list
	findinlist_letter('s');
	findinlist_letter('z');

	/* Iterate over all modules?
	 * Fails as struct module is not available (!exported) to module authors!
	 */
#if 0
		list_for_each_entry(module.list, THIS_MODULE, list)
			pr_info("module: %s\n", mymod->name);
#endif

	return 0;	/* success */
}

static void __exit list_exit(void)
{
	freelist();
	pr_info("removed\n");
}

module_init(list_init);
module_exit(list_exit);
