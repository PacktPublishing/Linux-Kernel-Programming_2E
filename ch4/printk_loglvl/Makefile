# ch4/printk_loglvl/Makefile
PWD          := $(shell pwd)
KDIR         := /lib/modules/$(shell uname -r)/build/
obj-m        += printk_loglvl.o

# Enable the pr_debug() and pr_devel() as well by removing the comment from
# one of the lines below
# (Note: EXTRA_CFLAGS deprecated; use ccflags-y)
#ccflags-y += -DDEBUG
#CFLAGS_printk_loglvl.o := -DDEBUG

all:
	make -C $(KDIR) M=$(PWD) modules
install:
	make -C $(KDIR) M=$(PWD) modules_install
clean:
	make -C $(KDIR) M=$(PWD) clean
