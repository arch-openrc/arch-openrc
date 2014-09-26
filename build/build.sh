#!/bin/bash
## build.sh: all the steps for building openrc and eudev packages

run_pre(){
    cd ..
    git clean -dfx  # Remove untracked directories and files
}

chroot_init(){
    local blacklist=('libsystemd')
    cd ${RUN_DIR} && cd ../${START_DIR}/${START_PKG}
    echo "==> building ${START_PKG}"
    sudo ${BRANCH}-${ARCH}-build -c -r ${CHROOT}
    sudo pacman -Rdd ${blacklist[@]} -r ${CHROOT}/${BRANCH}-${ARCH}/$USR --noconfirm
    sudo pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$USR --noconfirm
    cd ..
}

chroot_build(){
      for pkg in $(cat build-list); do
	cd $pkg
	echo "==> building $pkg"
	sudo makechrootpkg -n -r ${CHROOT}/${BRANCH}-${ARCH} || break
	cd ..
      done
}

mk_pkg(){
	run_pre

	case ${START_DIR} in
		openrc)
		  START_PKG=sysvinit
		  chroot_init
		  chroot_build
		  mk_sign
		  run_post
		;;
		eudev)
		  START_PKG=eudev
		  chroot_init
		  chroot_build
		  mk_sign
		  run_post
		;;
		* | all)
		  START_PKG=sysvinit
		  START_DIR=openrc
		  chroot_init
		  chroot_build
		  mk_sign
		  run_post
		  START_PKG=eudev
		  START_DIR=eudev
		  chroot_init
		  chroot_build
		  mk_sign
		  run_post
		;;
	esac
}

mk_sign(){

    cd ${RUN_DIR} && cd ../${START_DIR}/${START_PKG}
    signpkgs
    cd ..
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

    cd ${RUN_DIR} && cd ../${START_DIR}/${START_PKG}
    cp -v *.pkg.tar.xz{,.sig} $pkgdir
    cd ..
    for pkg in $(cat build-list); do
      cd $pkg
      cp -v *.pkg.tar.xz{,.sig} $pkgdir
      cd ..
    done

    # Perform any other commands
}

##############MAIN###################

export LANG=C
export LC_MESSAGES=C

RUN_DIR=$(pwd)

if [[ -f ./build.conf ]];then
    . "./build.conf"
else
    CHROOT=/opt/manjarobuild
fi

BRANCH=${1:-unstable}
ARCH=${2:-$(uname -m)}
START_DIR=${3:-all} # options: openrc,eudev,all
USR=$(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)

mk_pkg $1 $2 $3

#shutdown -h now
