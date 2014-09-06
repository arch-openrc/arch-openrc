#!/bin/bash
## mbuild.sh: build openrc and eudev packages for manajro

#set -x  # Used to Print commands and their arguments as they are executed

# switch to basic language
export LANG=C
export LC_MESSAGES=C

# Command line arguments
BRANCH=${1:-unstable}	# Branch to build for, defaulting to unstable
ARCH=${2:-$(uname -m)}	# Arch to build for, defaulting to system's arch
SIGN=${3:-0}  		# Whether to sign packages or not. 1 = sign
IS_EXTRA_EUDEV=${4:-0}  # since upower-0.9.23 is deprecated in upstream, it isn't expected to change

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

echo "==> Start building eudev"
date
#cd eudev/eudev-systemdcompat
user=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)

${BRANCH}-${ARCH}-build -c -r ${CHROOT}
#${BRANCH}-${ARCH}-build -r ${CHROOT}  # -c (Dont clean chroot everytime)
cd eudev

for pkg in $(cat build-list); do cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..; done
date
#makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
#pacman -Rdd libsystemd -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
#pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
#cd ../eudev
#makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
#pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
echo "==> Done building eudev"

echo "==> Start building openrc"
date
cd ../../openrc
for pkg in $(cat build-list); do cd $pkg && makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break && cd ..; done
date
echo "==> Done building openrc"

if (( $IS_EXTRA_EUDEV ));then
	echo "==> Start building upower-pm-utils"
	date
	cd ../eudev/upower-pm-utils
	makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH}
	date
	echo "==> Done building upower-pm-utils"
fi

### Sign packages
if [ "$SIGN" -eq 1 ]; then
	## eudev
	cd $cwd # Back to the base directory
	cd eudev
	for pkg in $(cat build-list); do cd $pkg && signpkgs || break && cd ..; done
	
	## openrc
	cd $cwd # Back to the base directory
	cd openrc
	for pkg in $(cat build-list); do cd $pkg && signpkgs || break && cd ..; done
fi

#shutdown -h now
