#!/bin/bash
# ch13/4_lockdep/lock_stats_demo.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Programming" 2E
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
#****************************************************************
# Brief Description:
# Simple script to demo some kernel locking statistics.
name=$(basename $0)

die()
{
echo >&2 "FATAL: $@"
exit 1
}

clear_lock_stats() {
  echo 0 > /proc/lock_stat
}
enable_lock_stats() {
  echo 1 > /proc/sys/kernel/lock_stat
}
disable_lock_stats() {
  echo 0 > /proc/sys/kernel/lock_stat
}
view_lock_stats() {
  cat /proc/lock_stat
}


#--- 'main'
[[ $(id -u) -ne 0 ]] && die "Needs root."

disable_lock_stats

#--- Test case --------------------------------------------------------
# We want to see some of our locks and their stats.
# So run our 'deadlock' test module but with no parameter (so, no deadlock occurs)
cd deadlock_eg_AB-BA
KMOD=deadlock_eg_AB-BA
KFUNC=sched_setaffinity
KFUNC_PTR=0x$(sudo grep -w "T ${KFUNC}" /proc/kallsyms |awk '{print $1}')
[[ -z "${KFUNC_PTR}" ]] && {
  echo "${name}: lookup of /proc/kallsyms failed, aborting..."
  exit 1
}
[[ ! -f ./${KMOD}.ko} ]] && make || exit 1
sudo rmmod ${KMOD} 2>/dev/null

# Enable lock stats only now!
clear_lock_stats
enable_lock_stats
sudo insmod ./${KMOD}.ko func_ptr=${KFUNC_PTR}
#----------------------------------------------------------------------

disable_lock_stats
REPFILE=lockstats.txt
view_lock_stats > ${REPFILE}
#view_lock_stats |tee ${REPFILE}
rmmod ${KMOD}
cd ..
echo "${name}: done, see the kernel locking stats in ${REPFILE}"
exit 0
