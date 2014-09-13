#!/bin/bash
## build.sh: all the steps for building openrc and eudev packages

# # Clean
# echo "Performing pre-build tasks..."
# ./prebuild.sh
#
# # Build
# sudo ./mbuild.sh
# #sudo ./mbuild.sh unstable i686
# #sudo ./mbuild.sh testing
# #sudo ./mbuild.sh testing i686
#
# # Sign
# echo "Signing packages..."
# ./msign.sh
# #sudo ./msign.sh
#
# # Copy and post install
# echo "Performing post-build tasks..."
# ./postbuild.sh

###########################################################################

#set -x

# switch to basic language
export LANG=C
export LC_MESSAGES=C

cwd=$(pwd)

if [[ -f ./build.conf ]];then
	. "./mbuild.conf"
else
	CHROOT=/opt/manjarobuild
fi

# do UID checking here so someone can at least get usage instructions
if [ "$EUID" != "0" ]; then
    echo "error: This script must be run as root."
    exit 1
fi

run_pre(){
  cd ..
  git clean -dfx  # Remove untracked directories and files
}



mk_pkg(){

  local BRANCH=${1:-unstable}	# Branch to build for, defaulting to unstable
  local ARCH=${2:-$(uname -m)}	# Arch to build for, defaulting to system's arch
  local IS_EXTRA_EUDEV=${3:-0}  # since upower-0.9.23 is deprecated in upstream, it isn't


user=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)
  cd $cwd
  echo "==> Start building eudev"
  date
  cd ../eudev/eudev
  ${BRANCH}-${ARCH}-build -c -r ${CHROOT}
  pacman -Rdd libsystemd systemd -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
  pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm
  cd $cwd
  cd ../eudev/eudev-systemdcompat
  makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break
  pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$user --noconfirm

  if (( $IS_EXTRA_EUDEV ));then
    cd $cwd
    cd ../eudev
    for pkg in $(cat build-list); do
      echo "==> Start building $pkg"
      cd $pkg
      makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break
      cd ..
      echo "==> Done building $pkg"
    done
  fi
  date
  echo "==> Done building eudev"

  echo "==> Start building openrc"
  date
  cd $cwd
  cd ../openrc
  for pkg in $(cat build-list); do
    cd $pkg
    makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break
    cd ..
  done
  date
  echo "==> Done building openrc"
}

mk_sign(){

  local IS_EXTRA_EUDEV=${3:-0}

  ## eudev
  cd ../eudev/eudev
  signpkgs
  cd $cwd
  cd ../eudev/eudev-systemdcompat
  signpkgs
  cd ..
  if (( $IS_EXTRA_EUDEV ));then
    for pkg in $(cat build-list); do
      cd $pkg
      signpkgs
      cd ..
    done
  fi

  ## openrc
  cd $cwd # Back to the base directory
  cd ../openrc
  for pkg in $(cat build-list); do
    cd $pkg
    signpkgs
    cd ..
  done
}

run_post(){

  local IS_EXTRA_EUDEV=${3:-0}

  # Specify a directory for storing built packages
  pkgdir="$cwd/packages/"

  # Check if pkgdir exists, if not then create it
  if [ ! -d "$pkgdir" ]; then
	  mkdir -p "$pkgdir"
  fi

  # Copy built packages and their sig files to $pkgdir
  ## eudev
  cd $cwd
  cd ../eudev/eudev
  cp -v *.pkg.tar.xz{,.sig} $pkgdir
  cd $cwd
  cd ../eudev/eudev-systemdcompat
  cp -v *.pkg.tar.xz{,.sig} $pkgdir

  if (( $IS_EXTRA_EUDEV ));then
    cd $cwd
    cd ../eudev
    for pkg in $(cat build-list); do
      cd $pkg
      cp -v *.pkg.tar.xz{,.sig} $pkgdir
      cd ..
    done
  fi

  ## openrc
  cd $cwd # Back to the base directory
  cd ../openrc
  for pkg in $(cat build-list); do
    cd $pkg
    cp -v *.pkg.tar.xz{,.sig} $pkgdir
    cd ..
  done

  # Perform any other commands
}

run_pre

mk_pkg $1 $2 $3

mk_sign $3

run_post $3



#shutdown -h now
