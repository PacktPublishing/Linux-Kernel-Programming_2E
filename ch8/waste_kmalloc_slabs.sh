#!/bin/bash

# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/ 
set -euo pipefail

name=$(basename $0)
[[ $(id -u) -ne 0 ]] && {
   echo "${name}: requires root"
   exit 1
}

[[ ! -f /sys/kernel/debug/slab/kmalloc-64/alloc_traces ]] && {
   echo "${name}: can't find the alloc-traces pseudofile(s); check:
- are you running kernel ver >= 6.1 (required) ?
- have you booted with kernel parameter slub_debug=FZPU ?"
   exit 1
}

TMPF=/tmp/t.$$
IFS=$'\n\t'
cmd="grep -r -H -w waste /sys/kernel/debug/slab/kmalloc-[1-9]*/alloc_traces"
n=0

echo "${name}: gathering data, please be patient ..."
for rec in $(eval ${cmd})
do
 #echo "rec = ${rec}"
 # Eg.
 # /sys/kernel/debug/slab/kmalloc-8/alloc_traces:     58 strndup_user+0x4a/0x70 waste=406/7 age=831078/831403/831933 pid=1-801 cpus=0-5 406
 # -----kmalloc-<foo> slab----------------------: num-times-requested func+start/len waste=num_wasted_total/num_wasted_eachtime pid=<...> cpus=<...> 

 # Get the total wastage (the x in waste=x/y; y = # bytes wasted each time):
 # Kernel modules contain an extra field, the module name [foo]
 # So get the field# of the column having the string 'waste=' ...
 waste_fieldnum=$(echo "${rec}" | awk '{for(i=1;i<=NF;i++) {if ($i ~ /waste=/) print i}}')
 # ... and then extract the absolute number of wasted bytes based on it
 # (of the form: waste=406/7, i.e., waste=num_wasted_total/num_wasted_eachtime)
 num_wasted_total=$(echo "${rec}" | awk -v fld=${waste_fieldnum} '{print $fld}'|cut -d= -f2|cut -d/ -f1)
 #echo "wstg_abs= ${wastage_abs}"
 echo "${rec} ${num_wasted_total}" >> ${TMPF}
 let n=n+1
 [[ $((n % 100)) -eq 0 ]] && echo -n "."
done
echo

# Separate out kernel internal kmalloc-* slabs from kernel modules slabs
grep -v "\[.*\]" ${TMPF} > kint.waste || true
grep "\[.*\]" ${TMPF} > kmods.waste || true

echo "======== Wastage (highest-to-lowest with duplicate lines eliminated) ========"
echo "--------------- kernel internal ----------------
   Top 10 wasters (in desc order). (To see all, lookup the full report here: kint.waste)"
[[ -s kint.waste ]] && {
# kernel internal - the 8th fields is the total number of 'waste' bytes
# (As the abs number of wasted bytes is already there in the output, we use awk
#  to eliminate the last col, the total number of wasted bytes)
# Also, use uniq(1) to eliminate duplicate lines (?)
sort -k8nr kint.waste | awk 'NF{NF--};1' | uniq | head
} || echo "-none-"

echo "
--------------- kernel modules ----------------
   Top 10 wasters (in desc order). (To see all, lookup the full report here: kmods.waste)"
# kernel modules - the 9th fields is the total number of 'waste' bytes
[[ -s kmods.waste ]] && {
sort -k9nr kmods.waste | awk 'NF{NF--};1' | uniq | head
} || echo "-none-"

exit 0
