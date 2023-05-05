#!/bin/bash
# kbuild.sh
# Simple kernel build script; minimally tested, YMMV!

# Turn on Bash 'strict mode'! V useful
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
NUMCORES=$(nproc)
JOBS=$((${NUMCORES}*2))

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
echo "Version: $(head Makefile)"

if [[ ${CONFIGURE} -eq 1 ]] ; then
  lsmod > /tmp/lsmod.now
  echo "[+] make LSMOD=/tmp/lsmod.now localmodconfig"
  make LSMOD=/tmp/lsmod.now localmodconfig
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
  echo "[+] make menuconfig "
  make menuconfig && echo || die "menuconfig failed"
  ls -l .config
else
  echo "[-] Skipping kernel configure, just running 'make oldconfig'"
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
fi

# Ensure config is sane; on Ubuntu, SYSTEM_REVOCATION_KEYS being enabled causes
# the build to fail..
echo
echo "[+] scripts/config --disable SYSTEM_REVOCATION_KEYS"
scripts/config --disable SYSTEM_REVOCATION_KEYS

echo
echo "[+] time make -j${JOBS}"
time make -j${JOBS} && echo || die "make <kernel> *failed*"
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
exit 0
