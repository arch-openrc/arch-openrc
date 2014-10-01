#!/bin/bash

. ./lib_devtools


chroot_create(){
    setarch "${arch}" \
	mkmanjaroroot \
	    -C "${pacman_conf}" -M "${makepkg_conf}" \
	    -a "${arch}" -b "${branch}" "${WORKDIR}/root" \
	    "${base_packages[@]}"
}

chroot_update(){
    msg "Updating chroot for [${branch}] (${arch})"
    setarch "${arch}" \
	mkmanjaroroot -b ${branch} -u ${WORKDIR}/root
    msg "Updated chroot for [${branch}] (${arch})"
}

chroot_init(){
    if ${clean_first} || [[ ! -d "${WORKDIR}" ]]; then
	msg "Creating chroot for [${branch}] (${arch})..."

	for copy in "${WORKDIR}"/*; do
	    [[ -d $copy ]] || continue
	    msg2 "Deleting chroot copy '$(basename "${copy}")'..."

	    exec 9>"$copy.lock"
	    if ! flock -n 9; then
		stat_busy "Locking chroot copy '$copy'"
		flock 9
		stat_done
	    fi

	    if [[ "$(stat -f -c %T "${copy}")" == btrfs ]]; then
		{ type -P btrfs && btrfs subvolume delete "${copy}"; } &>/dev/null
	    fi
	    rm -rf --one-file-system "${copy}"
	done
	exec 9>&-

	rm -rf --one-file-system "${WORKDIR}"
	mkdir -p "${WORKDIR}"
	 || abort
    else
	chroot_update || abort
    fi
}

get_user(){
    local user=$(ls ${WORKDIR} | cut -d' ' -f1 | grep -v root | grep -v lock)
    echo $user
}

chroot_mkpkg(){
    exec setarch ${arch} \
	makechrootpkg ${makechrootpkg_args} \
		      -b ${branch} -r ${WORKDIR} \
		      -- "${makepkg_args}"
}

chroot_build_set(){
    for pkg in $(cat ${profiledir}/${profile}.set); do
	cd $pkg
	chroot_mkpkg || break
	if [[ $pkg == 'eudev' ]]; then
	    local blacklist=('libsystemd') user=$(get_user)
	    pacman -Rdd "${blacklist[@]}" -r ${WORKDIR}/$(get_user) --noconfirm
	    local temp
	    if [[ -z $PKGDEST ]];then
		temp=$pkg
	    else
		temp=$pkgdir/$pkg
	    fi
	    pacman -U $temp*${ARCH}*pkg*z -r ${WORKDIR}/$(get_user) --noconfirm
	fi
	cd ..
    done
}

chroot_build(){
    cd ${profile}
	chroot_init
	chroot_mkpkg
    cd ..
}

get_profiles(){
    local prof=
    for p in $(ls ${profiledir}/*.set);do
	local temp=${p##*/}
	prof=${prof:-}${prof:+|}${temp%.set}
    done
    echo $prof
}

mk_pkgdir(){
	pkgdir=${rundir}/packages/${arch}
	if ! [[ -d ${pkgdir} ]];then
	    mkdir -pv ${pkgdir}
	fi
}

run(){
    eval "case ${profile} in
	$profiles)
	    chroot_init
	    msg 'Start building profile: ['${profile}']'
	    chroot_build_set ${profile}
	    msg 'Finished building profile: ['${profile}']'
	;;
	*)
	    chroot_build
	;;
    esac"
}

#################################MAIN##########################################

export LC_MESSAGES=C

shopt -s nullglob

arch=$(uname -m)
branch="unstable"

pacman_conf_arch='default'

chroots=/srv/manjarobuild
profile=udev
rundir=$(pwd)
base_packages=(base-devel)

clean_first=true
clean_build=false
sign=false
update_first=false
namcap=false
nodeps=false
info=false

makepkg_args=

usage() {
    echo "Usage: $0 [options] -- makepkg_args"
    echo '    -h                   This help'
    echo '    -a <arch>            Set arch'
    echo '    -b <branch>          Set branch'
    echo '    -p <profile>         Set profile or pkg'
    echo '    -r <dir>             Create chroots in this directory'
    echo '    -s                   Sign packages'
    echo '    -u                   Update the working copy of the chroot before building'
    echo '    -w                   Clean pkgbuilds dir'
    echo '    -v                   Verbose'
    echo '    -n                   Run namcap check'
    echo '    -c [default]         Create persistent chroot'
    echo ''
    echo ''
    exit 1
}

while getopts 'r:a:b:p:suwvnch' arg; do
    case "${arg}" in
	a) arch="$OPTARG" ;;
	b) branch="$OPTARG" ;;
	p) profile="$OPTARG" ;;
	r) chroots="$OPTARG" ;;
	s) sign=true ;;
	u) update_first=true ;;
	w) clean_build=true ;;
	v) info=true ;;
	n) namcap=true;;
	c) clean_first=false ;;
	h) usage ;;
    esac
done

#shift $((OPTIND-1))

makepkg_args="$makepkg_args ${*:$OPTIND}"



if [[ "$arch" == 'multilib' ]]; then
	pacman_conf_arch='multilib'
	arch='x86_64'
	branch="${branch}-multilib"
	base_packages+=(multilib-devel)
fi

if [[ -r ~/.makepkg.conf ]];then
    . ~/.makepkg.conf
    if [[ -n $PKGDEST ]];then
	pkgdir=$PKGDEST
    else
	mk_pkgdir
    fi
else
    [[ -r /etc/makepkg.conf ]] && . /etc/makepkg.conf
    if [[ -n $PKGDEST ]];then
	pkgdir=$PKGDEST
    else
	mk_pkgdir
    fi
fi

profiledir=${rundir}/sets
#profiledir="@sysconfdir@/sets"

profiles=$(get_profiles)

WORKDIR=${chroots}/${branch}-${arch}

pacman_conf="/usr/share/devtools/pacman-${pacman_conf_arch}.conf"
makepkg_conf="/usr/share/devtools/makepkg-${arch}.conf"

if ${namcap};then
  makechrootpkg_args='-n'
fi

if ${info};then
    msg "OPTARGS:"
    msg2 "arch: $arch"
    msg2 "branch: $branch"
    msg2 "profile: $profile"
    msg2 "chroots: $chroots"
    msg "FLAGS:"
    msg2 "clean_first: $clean_first"
    msg2 "update_first: $update_first"
    msg2 "sign: $sign"
    msg2 "clean_build: $clean_build"
    msg "SETS:"
    msg2 "profiles: $profiles"
    msg "PATHS:"
    msg2 "profiledir: $profiledir"
    msg2 "pacman_conf_arch: ${pacman_conf_arch}"
    msg2 "pacman_conf: ${pacman_conf}"
    msg2 "makepkg_conf: $makepkg_conf"
    msg2 "rundir: $rundir"
    msg "PKG:"
    msg2 "pkgdir: $pkgdir"
    msg2 "PKGDEST: $PKGDEST"
    msg2 "base_packages: ${base_packages[*]}"
    msg "MAKEPKGARGS:"
    msg2 "makepkg_args: ${makepkg_args}"
    msg2 "EXTRA_OPTIND: ${*:$OPTIND}"
fi

if ${clean_build};then
    . ./run_pre
    git_clean
fi

if [[ $EUID !=  0 ]]; then
    die 'This script must be run as root.'
else
    run $@
fi

. ./run_post

if [[ -z $PKGDEST ]];then
    cp_pkgs ${profile}
fi

if ${sign};then
    sign_pkgs
fi
