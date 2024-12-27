#!/bin/bash
# countem.sh
# ***************************************************************
# * This program is part of the source code released for the book
# *  "Linux Kernel Programming", 2nd Ed
# *  (c) Author: Kaiwan N Billimoria
# *  Publisher:  Packt
# *  GitHub repository:
# *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
# *
# * From: Ch 6 : Kernel and Memory Management Internals Essentials
# ****************************************************************
# * Brief Description:
# * [ADDED LATER]
# * Counts the total number of processes, user and kernel threads currently
# * alive on the system, and thus calculates and shows the number of user
# * and kernel mode stacks currently in existance.
# *
# * For details, please refer the book, Ch 6.
# ****************************************************************
set -euo pipefail

# First capture the info into temp files
TMPP=/tmp/.p
TMPT=/tmp/.t
ps -A > ${TMPP}
ps -LAf > ${TMPT}

total_prcs=$(wc -l ${TMPP}|awk '{print $1}')
printf "1. Total # of processes alive                        = %9d\n" ${total_prcs}
total_thrds=$(wc -l ${TMPT}|awk '{print $1}')
printf "2. Total # of threads alive                          = %9d\n" ${total_thrds}
# ps -LAf shows all kernel threads names (col 10) in square brackets; count 'em
total_kthrds=$(cat ${TMPT}|awk '{print $10}'|grep "^\["|wc -l)
printf "3. Total # of kernel threads alive                   = %9d\n" ${total_kthrds}
printf " (each kthread will have a kernel-mode stack)\n"
total_uthrds=$((${total_thrds}-${total_kthrds}))
printf "4. Thus, total # of user mode threads  = (2) - (3)   = %9d\n" ${total_uthrds}
printf " (each uthread will have both a user and kernel-mode stack)\n"
printf "5. Thus, total # of kernel-mode stacks = (3) + (4)   = %9d\n" $((${total_kthrds}+${total_uthrds}))

rm -f ${TMPP} ${TMPT}
exit 0
