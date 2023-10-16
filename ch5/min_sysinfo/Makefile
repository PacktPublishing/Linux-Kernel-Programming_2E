# ch5/lkm_template/Makefile
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Programming" 2E
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
#
# From: Ch 5 : Writing Your First Kernel Module LKMs, Part 2
# ***************************************************************
# Brief Description:
# A 'better' Makefile template for Linux LKMs (Loadable Kernel Modules); besides
# the 'usual' targets (the build, install and clean), we incorporate targets to
# do useful (and indeed required) stuff like:
#  - adhering to kernel coding style (indent+checkpatch)
#  - several static analysis targets (via sparse, gcc, flawfinder, cppcheck)
#  - two _dummy_ dynamic analysis targets (KASAN, LOCKDEP); just to remind you!
#  - a packaging (.tar.xz) target and
#  - a help target.
#
# To get started, just type:
#  make help
#
# For details, please refer the book, Ch 5, section
# 'A "better" Makefile template for your kernel modules'.
#
# AUTHOR : Kaiwan N Billimoria
# DESCRIPTION : A simple kernel module 'better' Makefile template
# LICENSE : Dual MIT/GPL
# VERSION : 0.2

#------------------------------------------------------------------
# IMPORTANT : Set FNAME_C to the kernel module name source filename (without .c).
# This enables you to use this Makefile as a template; just update this variable!
# As well, the MYDEBUG variable (see it below) can be set to 'y' or 'n' (no being
# the default)
FNAME_C := min_sysinfo
ifeq (,$(FNAME_C))
	$(error ERROR: you Must set the FNAME_C variable in the Makefile)
endif
#------------------------------------------------------------------

#--- To support cross-compiling for kernel modules
# For architecture (cpu) 'arch', invoke make as:
#  make ARCH=<arch> CROSS_COMPILE=<cross-compiler-prefix>
# F.e. to cross-compile for the AArch64:
#  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
#
# Alternately:
#  export ARCH=<arch>
#  export CROSS_COMPILE=<cross-compiler-prefix>
#  make
#
# The KDIR var is set to a sample path below; you're expected to update it on
# your box to the appropriate path to the kernel source tree for that arch.
ifeq ($(ARCH),arm)
  # *UPDATE* 'KDIR' below to point to the ARM Linux kernel source tree on your box
  KDIR ?= ~/arm_prj/kernel/linux
else ifeq ($(ARCH),arm64)
  # *UPDATE* 'KDIR' below to point to the ARM64 (Aarch64) Linux kernel source
  # tree on your box
  KDIR ?= ~/arm64_prj/kernel/linux-5.10.60
else ifeq ($(ARCH),powerpc)
  # *UPDATE* 'KDIR' below to point to the PPC64 Linux kernel source tree on your box
  KDIR ?= ~/ppc_prj/kernel/linux-5.4
else
  # 'KDIR' is the Linux 'kernel headers' package on your host system; this is
  # usually an x86_64, but could be anything, really (f.e. building directly
  # on a Raspberry Pi implies that it's the host)
  KDIR ?= /lib/modules/$(shell uname -r)/build
endif
#---

# Compiler
CC     := $(CROSS_COMPILE)gcc
#CC    := clang
STRIP := ${CROSS_COMPILE}strip

PWD            := $(shell pwd)
obj-m          += ${FNAME_C}.o

#--- Debug or production mode?
# Set the MYDEBUG variable accordingly to y/n resp. We keep it off (n) by default.
# (Actually, debug info is always going to be generated when you build the
# module on a debug kernel, where CONFIG_DEBUG_INFO is defined, making this
# setting of the ccflags-y (or the older EXTRA_CFLAGS) variable mostly redundant
# (besides the still useful -DDEBUG).
# This simply helps us influence the build on a production kernel, forcing
# generation of debug symbols, if so required. Also, realize that the DEBUG
# macro is turned on by many CONFIG_*DEBUG* options; hence, we use a different
# macro var name, MYDEBUG).
MYDEBUG := n
DBG_STRIP := y
ifeq (${MYDEBUG}, y)
# https://www.kernel.org/doc/html/latest/kbuild/makefiles.html#compilation-flags
# EXTRA_CFLAGS deprecated; use ccflags-y
  ccflags-y   += -DDEBUG -g -ggdb -gdwarf-4 -Wall -fno-omit-frame-pointer -fvar-tracking-assignments
  DBG_STRIP := n
else
  ccflags-y   += -UDEBUG
endif
# We always keep the dynamic debug facility enabled; this allows us to turn
# dynamically turn on/off debug printk's later... To disable it simply comment
# out the following line
ccflags-y   += -DDYNAMIC_DEBUG_MODULE
KMODDIR ?= /lib/modules/$(shell uname -r)

# Gain access to kernel configs
include $(KDIR)/.config

# Strip the module? Note:
# a) Only strip debug symbols else it won't load correctly
# b) WARNING! Don't strip modules when using CONFIG_MODULE_SIG* crytographic security
ifdef CONFIG_MODULE_SIG
  DBG_STRIP := n
endif
ifdef CONFIG_MODULE_SIG_ALL
  DBG_STRIP := n
endif
ifdef CONFIG_MODULE_SIG_FORCE
  DBG_STRIP := n
endif


all:
	@echo
	@echo '--- Building : KDIR=${KDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} ccflags-y="${ccflags-y}" MYDEBUG=${MYDEBUG} DBG_STRIP=${DBG_STRIP} ---'
	@${CC} --version|head -n1
	@echo
	make -C $(KDIR) M=$(PWD) modules
	if [ "${DBG_STRIP}" = "y" ]; then \
	   ${STRIP} --strip-debug ${FNAME_C}.ko ; \
	fi
install:
	@echo
	@echo "--- installing ---"
	@echo " [First, invoking the 'make' ]"
	make
	@echo
	@echo " [Now for the 'sudo make install' ]"
	sudo make -C $(KDIR) M=$(PWD) modules_install
	sudo depmod
	@echo " [If !debug and !(module signing), stripping debug info from ${KMODDIR}/extra/${FNAME_C}.ko]"
	if [ "${DBG_STRIP}" = "y" ]; then \
           sudo ${STRIP} --strip-debug ${KMODDIR}/extra/${FNAME_C}.ko ; \
	fi
nsdeps:
	@echo "--- nsdeps (namespace dependencies resolution; for possibly importing ns's) ---"
	make -C $(KDIR) M=$(PWD) nsdeps
clean:
	@echo
	@echo "--- cleaning ---"
	@echo
	make -C $(KDIR) M=$(PWD) clean
# from 'indent'; comment out if you want the backup kept
	rm -f *~

# Any usermode programs to build? Insert the build target(s) below


#--------------- More (useful) targets! -------------------------------
INDENT := indent

# code-style : "wrapper" target over the following kernel code style targets
code-style:
	make indent
	make checkpatch

# indent- "beautifies" C code - to conform to the the Linux kernel
# coding style guidelines.
# Note! original source file(s) is overwritten, so we back it up.
indent:
ifeq (,$(shell which indent))
	$(error ERROR: install indent first)
endif
	@echo
	@echo "--- applying kernel code style indentation with indent ---"
	@echo
	mkdir bkp 2> /dev/null; cp -f *.[chsS] bkp/
	${INDENT} -linux --line-length95 *.[chsS]
	  # add source files as required

# Detailed check on the source code styling / etc
checkpatch:
	make clean
	@echo
	@echo "--- kernel code style check with checkpatch.pl ---"
	@echo
	$(KDIR)/scripts/checkpatch.pl --no-tree -f --max-line-length=95 *.[ch]
	  # add source files as required

#--- Static Analysis
# sa : "wrapper" target over the following kernel static analyzer targets
sa:
	make sa_sparse
	make sa_gcc
	make sa_flawfinder
	make sa_cppcheck

# static analysis with sparse
sa_sparse:
ifeq (,$(shell which sparse))
	$(error ERROR: install sparse first)
endif
	make clean
	@echo
	@echo "--- static analysis with sparse ---"
	@echo
# If you feel it's too much, use C=1 instead
# NOTE: deliberately IGNORING warnings from kernel headers!
	make -Wsparse-all C=2 CHECK="/usr/bin/sparse --os=linux --arch=$(ARCH)" -C $(KDIR) M=$(PWD) modules 2>&1 |egrep -v "^\./include/.*\.h|^\./arch/.*\.h"

# static analysis with gcc
sa_gcc:
	make clean
	@echo
	@echo "--- static analysis with gcc ---"
	@echo
	make W=1 -C $(KDIR) M=$(PWD) modules

# static analysis with flawfinder
sa_flawfinder:
ifeq (,$(shell which flawfinder))
	$(error ERROR: install flawfinder first)
endif
	make clean
	@echo
	@echo "--- static analysis with flawfinder ---"
	@echo
	flawfinder *.[ch]

# static analysis with cppcheck
sa_cppcheck:
ifeq (,$(shell which cppcheck))
	$(error ERROR: install cppcheck first)
endif
	make clean
	@echo
	@echo "--- static analysis with cppcheck ---"
	@echo
	cppcheck -v --force --enable=all -i .tmp_versions/ -i *.mod.c -i bkp/ --suppress=missingIncludeSystem .

# Packaging: just generates a tar.xz of the source as of now
tarxz-pkg:
	rm -f ../${FNAME_C}.tar.xz 2>/dev/null
	make clean
	@echo
	@echo "--- packaging ---"
	@echo
	tar caf ../${FNAME_C}.tar.xz *
	ls -l ../${FNAME_C}.tar.xz
	@echo '=== package created: ../$(FNAME_C).tar.xz ==='
	@echo '        TIP: When extracting, to extract into a directory with the same name as the tar file, do this:'
	@echo '              tar -xvf ${FNAME_C}.tar.xz --one-top-level'

help:
	@echo '=== Makefile Help : additional targets available ==='
	@echo
	@echo 'TIP: Type make <tab><tab> to show all valid targets'
	@echo 'FYI: KDIR=${KDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} ccflags-y="${ccflags-y}" MYDEBUG=${MYDEBUG} DBG_STRIP=${DBG_STRIP}'

	@echo
	@echo '--- 'usual' kernel LKM targets ---'
	@echo 'typing "make" or "all" target : builds the kernel module object (the .ko)'
	@echo 'install     : installs the kernel module(s) to INSTALL_MOD_PATH (default here: /lib/modules/$(shell uname -r)/).'
	@echo '            : Takes care of performing debug-only symbols stripping iff MYDEBUG=n and not using module signature'
	@echo 'nsdeps      : namespace dependencies resolution; for possibly importing namespaces'
	@echo 'clean       : cleanup - remove all kernel objects, temp files/dirs, etc'

	@echo
	@echo '--- kernel code style targets ---'
	@echo 'code-style : "wrapper" target over the following kernel code style targets'
	@echo ' indent     : run the $(INDENT) utility on source file(s) to indent them as per the kernel code style'
	@echo ' checkpatch : run the kernel code style checker tool on source file(s)'

	@echo
	@echo '--- kernel static analyzer targets ---'
	@echo 'sa         : "wrapper" target over the following kernel static analyzer targets'
	@echo ' sa_sparse     : run the static analysis sparse tool on the source file(s)'
	@echo ' sa_gcc        : run gcc with option -W1 ("Generally useful warnings") on the source file(s)'
	@echo ' sa_flawfinder : run the static analysis flawfinder tool on the source file(s)'
	@echo ' sa_cppcheck   : run the static analysis cppcheck tool on the source file(s)'
	@echo 'TIP: use Coccinelle as well: https://www.kernel.org/doc/html/v6.1/dev-tools/coccinelle.html'

	@echo
	@echo '--- kernel dynamic analysis targets ---'
	@echo 'da_kasan   : DUMMY target: this is to remind you to run your code with the dynamic analysis KASAN tool enabled; requires configuring the kernel with CONFIG_KASAN On, rebuild and boot it'
	@echo 'da_lockdep : DUMMY target: this is to remind you to run your code with the dynamic analysis LOCKDEP tool (for deep locking issues analysis) enabled; requires configuring the kernel with CONFIG_PROVE_LOCKING On, rebuild and boot it'
	@echo 'TIP: Best to build a debug kernel with several kernel debug config options turned On, boot via it and run all your test cases'

	@echo
	@echo '--- misc targets ---'
	@echo 'tarxz-pkg  : tar and compress the LKM source files as a tar.xz into the dir above; allows one to transfer and build the module on another system'
	@echo '        TIP: When extracting, to extract into a directory with the same name as the tar file, do this:'
	@echo '              tar -xvf ${FNAME_C}.tar.xz --one-top-level'
	@echo 'help       : this help target'
