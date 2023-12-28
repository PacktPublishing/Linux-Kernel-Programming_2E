#!/bin/bash
# ch11/cgroups/cpu_constrain/cpu_manual/cgv2_cpu_ctrl.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Programming" 2E
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
# ****************************************************************
# Brief Description:
# A quick test case for cgroups v2 CPU controller.
# We execute our prime number generator program under specified CPU
# constraints.
#
# For details, pl refer to the book Ch 11.
#
# Ref:
# https://docs.kernel.org/admin-guide/cgroup-v2.html#cpu
name=$(basename $0)
TDIR=test_group

die()
{
  echo "${name}:FATAL: $*" 1>&2; exit 1
}

# cleanup
remove_our_cgroup()
{
[[ -d ${CGV2_MNT}/${TDIR} ]] && {
  echo "-cpu" > ${CGV2_MNT}/${TDIR}/cgroup.subtree_control #>/dev/null 2>&1
  echo "[+] Removing our (cpu) cgroup"
  # First, if any processes belong to it, kill 'em
  [[ ! -z "$(cat ${CGV2_MNT}/${TDIR}/cgroup.procs)" ]] && kill $(cat ${CGV2_MNT}/${TDIR}/cgroup.procs)
  echo "" > ${CGV2_MNT}/${TDIR}/cgroup.procs 2>/dev/null
  rmdir ${CGV2_MNT}/${TDIR} || die "removing the cgroup failed
  (This can take some time to propogate...)."
}
} # end remove_our_cgroup()

cpu_resctrl_try()
{
echo "[+] Launch the prime number generator process now ..."
#--- Run the job
PRGNAME=primegen
PRG=../primegen/primegen
MAX_PRIMES=1000000
MAX_TIME=5

pkill ${PRGNAME}  # any stale instance...
[[ ! -f ${PRG} ]] && {
  remove_our_cgroup
  die "Pl build the \"${PRGNAME}\" program and rerun"
}

# run it!
local cmd="${PRG} ${MAX_PRIMES} ${MAX_TIME} &"
echo $cmd
echo
eval ${cmd}

PRGPID=$(ps -A|grep ${PRGNAME}|head -n1|awk '{print $1}')
[[ -z "${PRGPID}" ]] && {
  remove_our_cgroup
  die "couldn't fetch program pid"
}
#echo "It's running (pid=${PRGPID})"
ps -A|grep ${PRGNAME}

echo "[+] Insert the ${PRGPID} process into our new CPU ctrl cgroup"                                   
echo "${PRGPID}" > ${CGV2_MNT}/${TDIR}/cgroup.procs

echo "cat ${CGV2_MNT}/${TDIR}/cgroup.procs"
cat ${CGV2_MNT}/${TDIR}/cgroup.procs
sleep 1
} # end cpu_resctrl_try()
                                                                       
setup_our_cgv2_cpu()
{
echo "[+] Creating a cgroup here: ${CGV2_MNT}/${TDIR}"
if [[ ! -d ${CGV2_MNT}/${TDIR} ]]; then
   mkdir ${CGV2_MNT}/${TDIR} || {
    remove_our_cgroup
    die "creating sub-dir ${CGV2_MNT}/${TDIR} failed..."
   }
fi

echo "[+] Adding a 'cpu' controller to it's cgroups v2 subtree_control file"                                   
echo "+cpu" > ${CGV2_MNT}/${TDIR}/cgroup.subtree_control || {
   remove_our_cgroup
   die "adding cpu controller failed, aborting. status=$?."
}

#ls -l ${CGV2_MNT}/${TDIR}

# From the kernel doc:
# cpu.max
# A read-write two value file which exists on non-root cgroups. The default is "max 100000".
# The maximum bandwidth limit. It's in the following format:
#   $MAX $PERIOD
# which indicates that the group may consume up to $MAX in each $PERIOD duration.
# "max" for $MAX indicates no limit. If only one number is written, $MAX is updated.

# In effect, all processes collectively in the sub-control group will be allowed
# to run for $MAX out of a period of $PERIOD; with MAX=200,000 and PERIOD=1,000,000
# we're effectively allowing all processes there to run for 0.2s out of a period of
# 1 second, i.e., utilizing 20% CPU bandwidth!
# The unit of $MAX and $PERIOD is microseconds.
local pct_cpu=$(bc <<< "scale=3; (${MAX}/${PERIOD})*100.0")
echo "
***
Now allowing ${MAX} out of a period of ${PERIOD} to all processes in this cgroup, i.e., ${pct_cpu}% !
***
"
echo "${MAX} ${PERIOD}" > ${CGV2_MNT}/${TDIR}/cpu.max || {
  remove_our_cgroup
  die "error! updating cpu.max for our sub-control group failed"
}
} # end setup_our_cgv2_cpu()

cgroupv2_support_verify()
{
echo "[+] Checking for cgroup v2 kernel support"
[[ -f /proc/config.gz ]] && {
  zcat /proc/config.gz |grep -i cgroup >/dev/null || die "cgroup support not builtin to kernel?? Aborting..."
}

mount |grep cgroup2 >/dev/null || die "cgroup2 filesystem not mounted? Pl mount it first; aborting..."
export CGV2_MNT=$(mount |grep cgroup2 |awk '{print $3}')
[[ -z "${CGV2_MNT}" ]] && die "cgroup v2 filesystem not acquired, aborting..." || \
 echo "${name}: detected cgroup2 fs here: ${CGV2_MNT}"
grep -w "cpu" ${CGV2_MNT}/cgroup.controllers >/dev/null || die "cpu controller not supported?
(didn't find 'cpu' in ${CGV2_MNT}/cgroup.controllers; configure the kernel to include the 'cpu' controller)."
}


#---"main" here                                                        
[[ $(id -u) -ne 0 ]] && die "need root."                                          
cgroupv2_support_verify
which bc >/dev/null || die "the 'bc' utility is  missing; pl install and retry"

MIN_BANDWIDTH_US=1000
PERIOD=1000000         # 1 million
RUN_CGV2_EXPLORE=0

[[ $# -eq 0 ]] && {
  echo "
Usage: ${name} max-to-utilize(us) [run-cgroupsv2_explore-script]
max-to-utilize : REQUIRED: This value (microseconds) is the max amount of time
  the processes in the control group we create will be allowed to utilize the
  CPU; it's relative to the period, which is set to the value ${PERIOD}.
  So, f.e., passing the value 300,000 (out of 1,000,000) implies a max CPU utiltization
  of 0.3 seconds out of 1 second (i.e., 30% utilization).
  The valid range for the \$MAX value is [${MIN_BANDWIDTH_US}-${PERIOD}].
run-cgroupsv2_explore-script : OPTIONAL: OFF by default.
  Passing 1 here has the script invoke our cgroupsv2_explore bash script, passing
  the new cgroup as the one to show details of."
 exit 1
}
MAX=$1
if [[ ${MAX} -lt ${MIN_BANDWIDTH_US} ]] || [[ ${MAX} -gt ${PERIOD} ]] ; then
  die "your value for MAX (${MAX}) is invalid; must be in the range [${MIN_BANDWIDTH_US}-${PERIOD}]"
fi
[[ $2 -eq 1 ]] && RUN_CGV2_EXPLORE=1
TD=$(pwd)

remove_our_cgroup
setup_our_cgv2_cpu
cpu_resctrl_try

SLEEP_TIME_ALLOW=$((${MAX_TIME}+1)) # seconds
echo "
............... sleep for ${SLEEP_TIME_ALLOW} s, allowing the program to execute ................
"

[[ ${RUN_CGV2_EXPLORE} -eq 1 ]] && ../../cgroupsv2_explore -p ${CGV2_MNT}/${TDIR}
sleep ${SLEEP_TIME_ALLOW}

sleep 1
remove_our_cgroup
exit 0
