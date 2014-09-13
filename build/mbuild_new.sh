#!/bin/bash

#set -x

# switch to basic language
export LANG=C
export LC_MESSAGES=C

BRANCH=${1:-unstable}	# Branch to build for, defaulting to unstable
ARCH=${2:-$(uname -m)}	# Arch to build for, defaulting to system's arch
IS_EXTRA_EUDEV=${3:-0}  # since upower-0.9.23 is deprecated in upstream, it isn't expected to change

if [[ -f ./mbuild.conf ]];then
	. "./mbuild.conf"
else
	CHROOT=/opt/manjarobuild
fi

# do UID checking here so someone can at least get usage instructions
if [ "$EUID" != "0" ]; then
    echo "error: This script must be run as root."
    exit 1
fi

user=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)

cd eudev/eudev
echo "==> Start building eudev"
date
${BRANCH}-${ARCH}-build -c -r ${CHROOT}
pacman -Rdd libsystemd -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
cd ..
if (( $IS_EXTRA_EUDEV ));then
  for pkg in $(cat build-list); do
    echo "==> Start building $pkg"
    cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..;
    echo "==> Done building $pkg"
  done
fi
date
echo "==> Done building eudev"

cd ../openrc
echo "==> Start building openrc"
date
for pkg in $(cat build-list); do
  cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..;
done
date
echo "==> Done building openrc"

#shutdown -h now
