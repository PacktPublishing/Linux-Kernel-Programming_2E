#!/bin/bash
# pkg_install4ubuntu_lkp.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Programming" 2E
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
#****************************************************************
# Brief Description:
# Helper script to install all required packages (as well as a few more
# possibly) for the Linux Kernel Programming 2E book.

function die
{
	echo >&2 "$@"
	exit 1
}
runcmd()
{
    [[ $# -eq 0 ]] && return
    echo "$@"
    eval "$@" || die "failed"
}

#--- 'main'
echo "$(date): beginning installation of required packages..."
# ASSUMPTION! root partition / is on /dev/sda1
spc1=$(df|grep sda1|awk '{print $3}')
echo "Disk space in use currently: ${spc1} KB"

runcmd sudo apt update
echo "-----------------------------------------------------------------------"
# ensure basic pkgs are installed!
runcmd sudo apt install -y \
	gcc make perl

# packages typically required for kernel build
runcmd sudo apt install -y \
	asciidoc binutils-dev bison build-essential flex libncurses5-dev ncurses-dev \
	libelf-dev libssl-dev pahole tar util-linux xz-utils zstd

echo "-----------------------------------------------------------------------"

# other packages...
# TODO : check if reqd
#sudo apt install -y bc bpfcc-tools build-essential \

runcmd sudo apt install -y \
	bc bpfcc-tools bsdextrautils \
	clang cppcheck cscope curl exuberant-ctags \
	fakeroot flawfinder \
	git gnome-system-monitor gnuplot hwloc indent kmod \
	libnuma-dev linux-headers-$(uname -r) linux-tools-$(uname -r) \
	man-db net-tools numactl openjdk-18-jre  \
	perf-tools-unstable psmisc python3-distutils  \
	rt-tests smem sparse stress sysfsutils \
	tldr-py trace-cmd tree tuna virt-what yad
echo "-----------------------------------------------------------------------"
#runcmd sudo apt install -y bc bpfcc-tools bsdmainutils clang cmake cppcheck cscope curl \
# dwarves exuberant-ctags fakeroot flawfinder git gnome-system-monitor gnuplot \
# hwloc indent kernelshark libnuma-dev libjson-c-dev linux-tools-$(uname -r) \
# net-tools numactl openjdk-16-jre openssh-server perf-tools-unstable psmisc \
# python3-distutils rt-tests smem sparse stress sysfsutils tldr-py trace-cmd \
# tree tuna virt-what

# Add yourself to the vboxsf group (to gain access to VirtualBox shared folders);
# will require you to log out and back in (or even reboot) to take effect
groups |grep -q -w vboxsf || runcmd sudo usermod -G vboxsf -a ${USER}

spc2=$(df|grep sda1|awk '{print $3}')
echo "Difference : ($spc2 - $spc1) : $((spc2-spc1)) KB"

runcmd sudo apt autoremove
exit 0
