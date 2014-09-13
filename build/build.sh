#!/bin/bash
## build.sh: all the steps for building openrc and eudev packages

run_pre(){
    cd ..
    git clean -dfx  # Remove untracked directories and files
}

mk_pkg(){

    #if [ "$EUID" == "0" ]; then

      local branch=${1:-unstable}
      local arch=${2:-$(uname -m)}
      local user=$(ls ${CHROOT}/${branch}-${arch} | cut -d' ' -f1 | grep -v root | grep -v lock)
      local blacklist='libsystemd systemd'

      date
      cd ${RUN_DIR} && cd ../openrc/${START_PKG}
      echo "==> building ${START_PKG}"
      sudo ${branch}-${arch}-build -c -r ${CHROOT}
      cd ${RUN_DIR} && cd ../openrc
      for pkg in $(cat build-list); do
	cd $pkg
	echo "==> building $pkg"
	sudo makechrootpkg -n -r ${CHROOT}/${branch}-${arch} || break
	cd ..
      done
      date

      date
      cd ${RUN_DIR} && cd ../eudev
      for pkg in $(cat build-list); do
	cd $pkg
	echo "==> building $pkg"
	sudo makechrootpkg -n -r ${CHROOT}/${branch}-${arch} || break
	if [[ $pkg == "eudev" ]];then
	  sudo pacman -Rdd ${blacklist[@]} -r ${CHROOT}/${branch}-${arch}/$user --noconfirm
	  sudo pacman -U *${arch}*pkg*z -r ${CHROOT}/${branch}-${arch}/$user --noconfirm
	fi
	cd ..
      done
      date

    #else
      #echo "error: This script must be run as root."
      #exit 1
    #fi
}

mk_sign(){

    cd ${RUN_DIR} && cd ../openrc/${START_PKG}
    signpkgs
    cd ..
    for pkg in $(cat build-list); do
      cd $pkg
      signpkgs
      cd ..
    done

    cd ${RUN_DIR} && cd ../eudev
    for pkg in $(cat build-list); do
      cd $pkg
      signpkgs
      cd ..
    done
}

run_post(){

    local pkgdir="${RUN_DIR}/packages/"

    if [ ! -d "$pkgdir" ]; then
      mkdir -p "$pkgdir"
    fi

    cd ${RUN_DIR} && cd ../openrc/${START_PKG}
    cp -v *.pkg.tar.xz{,.sig} $pkgdir
    cd ${RUN_DIR} && cd ../openrc
    for pkg in $(cat build-list); do
      cd $pkg
      cp -v *.pkg.tar.xz{,.sig} $pkgdir
      cd ..
    done

    cd ${RUN_DIR} && cd ../eudev
    for pkg in $(cat build-list); do
      cd $pkg
      cp -v *.pkg.tar.xz{,.sig} $pkgdir
      cd ..
    done

    # Perform any other commands
}

##############MAIN###################

#set -x

export LANG=C
export LC_MESSAGES=C

RUN_DIR=$(pwd) # Back to the base directory

if [[ -f ./build.conf ]];then
    . "./build.conf"
else
    CHROOT=/opt/manjarobuild
    START_PKG=sysvinit
fi

run_pre

mk_pkg $1 $2

mk_sign

run_post



#shutdown -h now
