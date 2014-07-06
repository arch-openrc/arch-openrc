#!/bin/bash

# switch to basic language
export LANG=C
export LC_MESSAGES=C

if [[ -f mbuild.conf ]];then
	. "mbuild.conf"
else
	CHROOT=/opt/manjarobuild
fi

BRANCH=${1:-unstable}	# Allows specifying command line agrument for branch, defaulting to unstable
ARCH=${2:-$(uname -m)}	# Allows specifying command line agrument for arch, defaulting to system's arch

# Example of the command line arguments: sudo ./mbuild.sh testing i686

# do UID checking here so someone can at least get usage instructions
if [ "$EUID" != "0" ]; then
    echo "error: This script must be run as root."
    exit 1
fi

echo "==> Start building eudev"
date
cd eudev/libsystemd-eudevcompat
${BRANCH}-${ARCH}-build -c -r ${CHROOT}
user=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)
makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
pacman -Rdd libsystemd -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
cd ../eudev
makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
echo "==> Done building eudev"

echo "==> Start building openrc"
date
cd ../../openrc
for pkg in $(cat build-list); do cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..; done
date
echo "==> Done building openrc"

echo "==> Start building eudev (additional packages)"
date
cd ../../eudev
for pkg in $(cat build-list); do cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..; done
date
echo "==> Done building eudev (additional packages)"
#shutdown -h now
