#!/bin/bash
# kbuild.sh
# ***************************************************************
# * This program is part of the source code released for the book
# *  "Linux Kernel Programming", 2nd Ed
# *  (c) Author: Kaiwan N Billimoria
# *  Publisher:  Packt
# *  GitHub repository:
# *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
# *
# * From: Ch 3 : Building the 6.x Linux Kernel from Source - Part 2
# ****************************************************************
# * Brief Description:
# Simple kernel build script; minimally tested, YMMV!
# ****************************************************************

# Turn on Bash 'strict mode'! V useful to catch potential bugs/issues early.
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
NUMCORES=$(nproc)
JOBS=$((${NUMCORES}*2))
LOG=kbuild_log

CONFIGURE=1
BUILD_INSTALL_MOD=1

die()
{
echo >&2 "$@"
exit 1
}


#--- 'main'
[[ $# -ne 1 ]] && die "Usage: ${name} path-to-kernel-src-tree-to-build"
set +u
[[ ! -z "${ARCH}" ]] && {
	echo "${name}: you seem to want to build the kernel for arch ${ARCH}"
	echo "This simple script currently supports only x86_64"
	exit 1
}
set -u
cd $1 || exit 1
rm -f ${LOG} 2>/dev/null
(
date
echo "[Logging to file ${LOG} ...]"
echo "Version: $(head Makefile)"
) 2>&1 | tee -a ${LOG}

if [[ ${CONFIGURE} -eq 1 ]] ; then
  (
  lsmod > /tmp/lsmod.now
  echo "[+] make LSMOD=/tmp/lsmod.now localmodconfig"
  make LSMOD=/tmp/lsmod.now localmodconfig
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
  echo "[+] make menuconfig "
  ) 2>&1 | tee -a ${LOG}
  # menuconfig and tee (w/ stderr too redirected) don't seem to get along..
  make menuconfig && echo || die "menuconfig failed"
  ls -l .config 2>&1 | tee -a ${LOG}
else
  (
  echo "[-] Skipping kernel configure, just running 'make oldconfig'"
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
  ) 2>&1 | tee -a ${LOG}
fi

(
# Ensure config is sane; on recent Ubuntu (~ kernel ver >= 5.13),
# SYSTEM_REVOCATION_KEYS being enabled causes the build to fail..
echo
echo "[+] scripts/config --disable SYSTEM_REVOCATION_KEYS"
scripts/config --disable SYSTEM_REVOCATION_KEYS

echo
date
echo "[+] time make -j${JOBS}"
time make -j${JOBS} && {
  echo
  date
} || die "make <kernel> *failed*"
[[ ! -f arch/x86/boot/bzImage ]] && die "make <kernel> *failed*? arch/x86/boot/bzImage not gen."

if [[ ${BUILD_INSTALL_MOD} -eq 1 ]] ; then
	echo
	echo "[+] sudo make modules_install "
	sudo make modules_install || die "*Failed* modules install"
else
	echo "[-] Skipping kernel modules build and install step"
fi

echo
echo "[+] sudo make install"
sudo make install || die "*Failed*"
echo "
Done, reboot, select your new kernel from the bootloader menu & boot it
(If not already done, you first need to configure GRUB to show the menu at boot)"
date
) 2>&1 | tee -a ${LOG}
exit 0
