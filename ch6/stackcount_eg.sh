#!/bin/bash
# Linux Kernel Programming, 2E
# Simple wrapper script to try out stackcount[-bpfcc].
# Lightly tested ONLY for Ubuntu x86_64...
# TODO:
# [ ] work properly on Fedora distro..
#
# For details, refer to the book, Ch 6.
# (c) Kaiwan NB, MIT
name=$(basename $0)
set -euo pipefail
SEP="--------------------------------------------------------------------------------"
# Ubuntu specific name for BCC tool(s), adjust as required for other distros
PRG=stackcount
which ${PRG} >/dev/null 2>&1 || {
	which ${PRG}-bpfcc >/dev/null 2>&1 && PRG=stackcount-bpfcc || {
		echo "FATAL: requires stackcount[-bpfcc] to be installed."
		exit 1
	}
}

which ${PRG} >/dev/null
[ $? -ne 0 ] && {
  echo "${name}: oops, ${PRG} not installed? aborting..."
  exit 1
}

doit()
{
[ $# -lt 2 ] && return
echo "${SEP}
${2}
cmd: ${1}
${SEP}"
eval ${1}
}

HDR="${name}: NOTE: Lightly tested ONLY for Ubuntu x86_64
(Patience please, it can take a while...)
"
echo "${HDR}"

echo "Running ping in the background..."
ping yahoo.com >/dev/null 2>&1 &

cmd="sudo ${PRG} --regex 'net_[rt]x_action' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show net_rx_action() and net_tx_action() (the NET_RX_SOFTIRQ and NET_TX_SOFTIRQ softirq's) call stacks:"
doit "${cmd}" "${desc}"

cmd="sudo ${PRG} 'ip_output' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show ip_output() call stacks:"
doit "${cmd}" "${desc}"

cmd="sudo ${PRG} 'dev_hard_start_xmit' --duration 3 --perpid --delimited 2>/dev/null"
# With --verbose, even the virtual address of the text (code) function in question is printed on the left
#cmd="sudo ${PRG} --verbose 'dev_hard_start_xmit' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show dev_hard_start_xmit() call stacks:"
doit "${cmd}" "${desc}"

pkill ping

[[ 0 -eq 1 ]] && {
# Show sched context switch call stacks (off by default here)
cmd="sudo ${PRG} t:sched:sched_switch --duration 1 --perpid --delimited 2>/dev/null"
desc="Show CPU scheduler context switch call stacks:"
doit "${cmd}" "${desc}"
}
exit 0
