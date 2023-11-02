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
LOG=kbuild_log

CONFIGURE=0
BUILD_INSTALL_MOD=ask

die()
{
echo >&2 "$@"
exit 1
}

# get_yn_reply
# User's reply should be Y or N.
# Returns:
#  0  => user has answered 'Y'
#  1  => user has answered 'N'
get_yn_reply()
{
echo -n "Type Y or N please (followed by ENTER) : "
local str="${@}"
while true
do
   echo ${str}
   read reply

   case "$reply" in
        y | yes | Y | YES ) return 0
                        ;;
        n* | N* ) return 1
                        ;;
        *) echo "What? Pl type Y / N"
   esac
done
}

usage()
{
echo "Usage: ${name} [-c] path-to-kernel-src-tree-to-build
 -c : configure the kernel to a default state (via the 'localmodconfig' approach)
 NOTE: this will overwrite your existing .config !"
}

configure_kernel_localmodconfig()
{
  lsmod > /tmp/lsmod.now
  echo "[+] make LSMOD=/tmp/lsmod.now localmodconfig"
  make LSMOD=/tmp/lsmod.now localmodconfig
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
  echo "[+] make menuconfig "
  # menuconfig and tee (w/ stderr too redirected) don't seem to get along..
  make menuconfig && echo || die "menuconfig failed"
  ls -l .config 2>&1 | tee -a ${LOG}

 # Ensure config is sane; on recent Ubuntu (~ kernel ver >= 5.13),
 # SYSTEM_REVOCATION_KEYS being enabled causes the build to fail..
 echo
 echo "[+] scripts/config --disable SYSTEM_REVOCATION_KEYS"
 scripts/config --disable SYSTEM_REVOCATION_KEYS
}

build_kernel()
{
 local nproc=$(nproc)
 if [[ ${nproc} -le 64 ]] ; then
   JOBS=$(($(nproc)*2))
 else
   JOBS=$(bc <<< "scale=0; (${nproc}*1.5)")
   JOBS=${JOBS::-2} # strip the .0
 fi

 echo
 date
 echo "[+] time make -j${JOBS}"
 time make -j${JOBS} && {
   echo
   date
 } || die "make <kernel> *failed*"
 [[ ! -f arch/x86/boot/bzImage ]] && die "make <kernel> *failed*? arch/x86/boot/bzImage not generated" || true
}

install_modules()
{
	echo
	echo "[+] sudo make modules_install "
	sudo make modules_install || die "*Failed* modules install"
}


#--- 'main'
[[ $# -lt 1 ]] && {
	usage ; exit 1
}
set +u
if [[ ! -z "${ARCH}" ]] && [[ "${ARCH}" != "x86" ]] ; then
	echo "${name}: you seem to want to build the kernel for arch ${ARCH}"
	echo "This simple script currently supports only x86_64"
	exit 1
fi
set -u

# Args processing
if [[ $# -ge 1 ]] && [[ "$1" = "-h" ]] ; then
	usage ; exit 0
fi
[[ $# -eq 1 ]] && KSRC=$1
[[ $# -eq 2 ]] && KSRC=$2
if [[ $# -lt 2 ]] && [[ "$1" = "-c" ]] ; then
	usage ; exit 1
fi
cd ${KSRC} || exit 1

rm -f ${LOG} 2>/dev/null
(
date
echo "[Logging to file ${LOG} ...]"
echo "Version: $(head Makefile)"
) 2>&1 | tee -a ${LOG}

if [[ $# -ge 2 ]] && [[ "$1" = "-c" ]] ; then
	CONFIGURE=1
	configure_kernel_localmodconfig 2>&1 | tee -a ${LOG}
else
  (
  echo "[-] Skipping kernel configure, using existing .config
    (Tip: pass -c if you want to configure the kernel to defaults)"
  [[ ! -f .config ]] && {
	echo "Fresh kernel (no .config), so running 'make oldconfig'"
	echo "[+] make oldconfig"
	make oldconfig  # Update current config utilising a provided .config as base
  } || true
  ) 2>&1 | tee -a ${LOG}
fi

build_kernel 2>&1 | tee -a ${LOG}

(
#--- Modules install?
echo "
Install kernel modules now?"
set +e
get_yn_reply
stat=$?
set -e
if [[ ${stat} -eq 0 ]] ; then
	install_modules
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
