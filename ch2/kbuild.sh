#!/bin/bash
# kbuild.sh
# Simple kernel build script; minimally tested, YMMV!
name=$(basename $0)
NUMCORES=$(nproc)
JOBS=$((${NUMCORES}*2))

CONFIGURE=1
BUILD_INSTALL_MOD=1

[ $# -ne 1 ] && {
	echo "Usage: ${name} path-to-kernel-src-tree-to-build"
	exit 1
}
[ ! -z "${ARCH}" ] && {
	echo "${name}: you seem to want to build the kernel for arch ${ARCH}"
	echo "This simple script currently supports only x86_64"
	exit 1
}
cd $1 || exit 1
echo "Version: $(head Makefile)"

if [ ${CONFIGURE} -eq 1 ] ; then
  lsmod > /tmp/lsmod.now
  echo "[+] make LSMOD=/tmp/lsmod.now localmodconfig"
  make LSMOD=/tmp/lsmod.now localmodconfig
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
  echo "[+] make menuconfig "
  make menuconfig && echo || {
    echo "menuconfig failed"
    exit 1
  }
  ls -l .config
else
  echo "[Skipping kernel configure, just running 'make oldconfig']"
  echo "[+] make oldconfig"
  make oldconfig  # Update current config utilising a provided .config as base
fi
# Ensure config is sane
echo "[+] scripts/config --disable SYSTEM_REVOCATION_KEYS"
scripts/config --disable SYSTEM_REVOCATION_KEYS

echo "[+] time make -j${JOBS}"
time make -j${JOBS} && echo || {
  echo "make <kernel> fail"
  exit 1
}
[ ! -f arch/x86/boot/bzImage ] && {
  echo "make <kernel> fail? arch/x86/boot/bzImage not gen."
  exit 1
}

if [ ${BUILD_INSTALL_MOD} -eq 1 ] ; then
   echo "[+] sudo make modules_install "
   sudo make modules_install || { 
     echo "Fail modules install"
     exit 1
   }
else
  echo "[Skipping kernel modules build and install step]"
fi

echo "[+] sudo make install"
sudo make install || {
	echo Fail.
	exit 1
}
echo "
Done, reboot, select your new kernel from the bootloader menu & boot it"
exit 0
