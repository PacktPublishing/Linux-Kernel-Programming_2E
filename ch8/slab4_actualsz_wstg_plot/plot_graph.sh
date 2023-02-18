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
#    file plotdata.txt after this :-)
# (To save you the trouble, we've (also) kept a sample plotdata.txt file in
#  the repo).

# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/ 
set -euo pipefail

name=$(basename $0)
TMPFILE=/tmp/plotdata
OUTFILE=plotdata.txt
PAUSE_TO_SHOW=1
IMAGE_NAME=graph.jpg

prep_datafile()
{
sudo dmesg > ${TMPFILE}
cut -c16- ${TMPFILE} | grep -v -i "^[a-z]" > ${OUTFILE}
# trim whitespace to one space
tr -s ' ' < ${OUTFILE} > /tmp/$$.1
# replace space with separator char (,)
tr ' ' ',' < /tmp/$$.1 > ${OUTFILE}
rm -f ${TMPFILE}
echo "Done, generated data file for gnuplot: ${OUTFILE}"
ls -l ${OUTFILE}
}

plotit()
{
local TITLE="Slab/Page Allocator: Requested vs Actually allocated size Wastage in Percent"
local SCALE="1:50"
local PLOTCMD="plot '${OUTFILE}' using ${SCALE} with lines title '${TITLE}',\
		'${OUTFILE}' title 'datafile: ${OUTFILE}'\
		with linespoints"
			 # the 2nd 'title ...' here is for the Legend
[ ${PAUSE_TO_SHOW} -eq 1 ] && PLOTCMD="${PLOTCMD}; \
 pause -1;"
#echo "PLOTCMD=${PLOTCMD}"

gnuplot -e \
	"set title \"${TITLE}\"; \
	set xlabel '{/:Bold Required Size (bytes)}' ; \
	set ylabel '{/:Bold Wastage incurred (%age)}' ; \
	set grid; \
	set ytics nomirror; \
	set datafile separator \",\"; 
	set xrange [*:*]; \
	set yrange [*:*]; \
	${PLOTCMD}; \
	set terminal jpeg size 1024,768; \
	set output '${IMAGE_NAME}'; \
	replot; \
	" 2>/dev/null

	[[ -f ${IMAGE_NAME} ]] && echo "Graph saved as this image: ${IMAGE_NAME}"
}


#--- 'main'
hash gnuplot || {
	echo "${name}: first install gnuplot"
	exit 1
}
pgrep Xorg >/dev/null || echo "${name}: WARNING: are you sure you're running in a GUI? (no Xorg detected)"
prep_datafile
plotit

exit 0
