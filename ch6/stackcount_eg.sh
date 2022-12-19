#!/bin/bash
# Linux Kernel Programming, 2E
# Simple wrapper script to try out stackcount[-bpfcc].
# For details, refer to the book, Ch 6.
# (c) Kaiwan NB, MIT
set -euo pipefail
SEP="--------------------------------------------------------------------------------"
# Ubuntu specific name for BCC tool(s), adjust as required for other distros
PRG=stackcount-bpfcc
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
${SEP}"
eval ${1}
}

echo "Running ping in the background..."
ping altavista.com >/dev/null 2>&1 &

cmd="sudo ${PRG} --regex 'net_[tr]x_action' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show net_rx_action() and net_tx_action() (the NET_RX_SOFTIRQ and NET_TX_SOFTIRQ softirq's) call stacks:"
doit "${cmd}" "${desc}"

cmd="sudo ${PRG} 'ip_output' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show ip_output() call stacks:"
doit "${cmd}" "${desc}"

cmd="sudo ${PRG} 'dev_hard_start_xmit' --duration 3 --perpid --delimited 2>/dev/null"
desc="Show dev_hard_start_xmit() call stacks:"
doit "${cmd}" "${desc}"

pkill ping

#cmd="sudo ${PRG} t:sched:sched_switch --duration 1 --perpid --delimited 2>/dev/null"
#desc="Show CPU scheduler context switch  call stacks:"
#doit "${cmd}" "${desc}"
