#!/bin/bash
## mbuild.sh: build openrc and eudev packages for manajro

#set -x  # Used to Print commands and their arguments as they are executed

# switch to basic language
export LANG=C
export LC_MESSAGES=C

# Command line arguments
BRANCH=${1:-unstable}	# Branch to build for, defaulting to unstable
ARCH=${2:-$(uname -m)}	# Arch to build for, defaulting to system's arch
IS_EXTRA_EUDEV=${3:-0}  # since upower-0.9.23 is deprecated in upstream, it isn't expected to change

# Parse config file for mbuild
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

# Keep track of the base build directory
cwd=`pwd`

# Dont know what this does
user=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)

# Setup chroot
${BRANCH}-${ARCH}-build -c -r ${CHROOT} | exit 1  # (Clean previous chroot)
#${BRANCH}-${ARCH}-build -r ${CHROOT} | exit 1  # (Dont clean chroot everytime)

echo "==> Start building eudev"
date
#cd eudev/eudev-systemdcompat
cd ../eudev
for pkg in $(cat build-list); do echo "Building $pkg"; cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} | break && cd ..; done
date
echo "==> Done building eudev"
#makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
#pacman -Rdd libsystemd -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
#pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
#cd ../eudev
#makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
#pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm

echo "==> Start building openrc"
date
cd "$cwd"
cd ../openrc
for pkg in $(cat build-list); do echo "Building $pkg"; cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} | break && cd ..; done
date
echo "==> Done building openrc"

if (( $IS_EXTRA_EUDEV ));then
	echo "==> Start building upower-pm-utils"
	date
	cd "$cwd"	
	cd ../eudev/upower-pm-utils
	makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
	date
	echo "==> Done building upower-pm-utils"
fi

## Sign packages
# Moved to the msign.sh script

#shutdown -h now
