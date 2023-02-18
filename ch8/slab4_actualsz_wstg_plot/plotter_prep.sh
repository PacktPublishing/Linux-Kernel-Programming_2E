#!/bin/bash
# plotter_prep.sh
# Book: Linux Kernel Programming 2E, Kaiwan N Billimoria, Packt.
# Part of the ch8/slab4_actualsz_wstg_plot code.
#
# Script to help prepare the data file from kernel log.
# We assume that:
# a) sudo dmesg -C   ; was done prior to the kernel module being inserted
#    (clearing any stale messages from the kernel log)
# b) The cut(1) below gets rid of the dmesg timestamp (the first column); we
#    assume the first col is the timestamp
# c) You will comment out or delete any extraneous lines in the final o/p
#    file 2plotdata.txt after this :-)
# (To save you the trouble, we've (also) kept a sample 2plotdata.txt file in
#  the repo).

# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/ 
set -euo pipefail

TMPFILE=/tmp/plotdata
OUTFILE=2plotdata.txt

sudo dmesg > ${TMPFILE}
cut -c16- ${TMPFILE} | grep -v -i "^[a-z]" > ${OUTFILE}
rm -f ${TMPFILE}
echo "Done, data file for gnuplot is ${OUTFILE}
(follow the steps in the LKP book, Ch 8, to plot the graph)."
ls -l ${OUTFILE}
