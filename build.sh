#!/bin/bash
## build.sh: all the steps for building openrc and/or eudev packages

run_clean(){
	cd ..
	git clean -dfx  # Remove untracked directories and files
}

get_user(){
	echo $(ls ${CHROOT}/${BRANCH}-${ARCH} | cut -d' ' -f1 | grep -v root | grep -v lock)
}

rm_blacklisted(){
	local blacklist=('libsystemd')
	sudo pacman -Rdd ${blacklist[@]} -r ${CHROOT}/${BRANCH}-${ARCH}/$(get_user) --noconfirm
}

install_built(){
	sudo pacman -U *${ARCH}*pkg*z -r ${CHROOT}/${BRANCH}-${ARCH}/$(get_user) --noconfirm
}

chroot_init(){
	cd ${RUN_DIR} && cd ../${START_DIR}/${START_PKG} && echo "==> building ${START_PKG}"
	sudo "${BRANCH}-${ARCH}-build" -c -r ${CHROOT}
	rm_blacklisted && install_built
	cd ..
}

chroot_pkg(){
	for pkg in $(cat build-list); do
		cd $pkg && echo "==> building $pkg"
		sudo makechrootpkg -n -b ${BRANCH} -r ${CHROOT}/${BRANCH}-${ARCH} || break
		cd ..
	done
}

# $1: task
run_task(){

	cd ${RUN_DIR} && cd ../${START_DIR}/${START_PKG} && $1 && cd ..
	for pkg in $(cat build-list); do
		cd $pkg && $1 && cd ..
	done
}

cp_pkg(){
	if [ ! -d "${RUN_DIR}/packages/" ]; then
		mkdir -p "${RUN_DIR}/packages/"
	fi
	cp *.pkg.tar.xz{,.sig} ${RUN_DIR}/packages/
}

mk_pkg(){
	run_clean
	case ${START_DIR} in
		openrc)
			START_PKG=sysvinit
			chroot_init && chroot_pkg
			run_task signpkgs && run_task cp_pkg
		;;
		eudev)
			START_PKG=eudev
			chroot_init && chroot_pkg
			run_task signpkgs && run_task cp_pkg
		;;
		* | all)
			START_PKG=sysvinit && START_DIR=openrc
			chroot_init && chroot_pkg
			mk_sign && run_post
			START_PKG=eudev && START_DIR=${START_PKG}
			chroot_init && chroot_pkg
			run_task signpkgs && run_task cp_pkg
		;;
	esac
}

########################MAIN################################

export LANG=C
export LC_MESSAGES=C

RUN_DIR=$(pwd)

BRANCH='unstable'
ARCH=$(uname -m)
START_DIR='all' # all,eudev,openrc
CHROOT='/srv/manjarobuild'

usage() {
	echo 'Usage: build.sh [options]'
	echo ' options:'
	echo '    -b <branch>   [default] '"${BRANCH}"
	echo '    -a <arch>     [default] '"${ARCH}"
	echo '    -s <startdir> [default] '"${START_DIR}"
	echo '    -r <chroot>   [default] '"${CHROOT}"
	echo '    -h            Help'
	exit 1
}

while getopts ':b:a:s:r:' arg; do
	case "${arg}" in
		b) BRANCH=$OPTARG ;;
		a) ARCH=$OPTARG ;;
		s) START_DIR=$OPTARG ;;
		r) CHROOT=$OPTARG ;;
		h|*) usage ;;
	esac
done
shift $((OPTIND-1))

mk_pkg $@

############################################################

# BRANCH=${1:-unstable}
# ARCH=${2:-$(uname -m)}
# START_DIR=${3:-all} # options: openrc,eudev,all
#
# mk_pkg $1 $2 $3

#############################################################
