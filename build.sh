#!/bin/sh

msg() {
    local mesg=$1; shift
    printf "${GREEN}==>${ALL_OFF}${BOLD} ${mesg}${ALL_OFF}\n" "$@" >&2
}

msg2() {
    local mesg=$1; shift
    printf "${BLUE}  ->${ALL_OFF}${BOLD} ${mesg}${ALL_OFF}\n" "$@" >&2
}

msg3() {
    local mesg=$1 mesg2=$2; shift
    printf "${YELLOW}==>${ALL_OFF}${BOLD} ${mesg}${ALL_OFF}${GREEN} ${mesg2}${ALL_OFF}\n" "$@" >&2
}

error() {
    local mesg=$1; shift
    printf "${RED}==> ERROR:${ALL_OFF}${BOLD} ${mesg}${ALL_OFF}\n" "$@" >&2
}

stat_busy() {
    local mesg=$1; shift
    printf "${GREEN}==>${ALL_OFF}${BOLD} ${mesg}...${ALL_OFF}" >&2
}

stat_done() {
    printf "${BOLD}done${ALL_OFF}\n" >&2
}

cleanup() {
    [[ -n $WORKDIR ]] && rm -rf "$WORKDIR"
    [[ $1 ]] && exit $1
}

abort() {
    msg 'Aborting...'
    cleanup 0
}

trap_abort() {
    trap - EXIT INT QUIT TERM HUP
    abort
}

trap_exit() {
    trap - EXIT INT QUIT TERM HUP
    cleanup
}

die() {
    error "$*"
    cleanup 1
}

trap 'trap_abort' INT QUIT TERM HUP
trap 'trap_exit' EXIT

chroot_init(){
    if ${clean_first} || [[ ! -d "${workdir}" ]]; then
	msg "Creating chroot for [${branch}] (${arch})..."

	for copy in "${workdir}"/*; do
	    [[ -d $copy ]] || continue
	    msg2 "Deleting chroot copy '$(basename "${copy}")'..."

	    exec 9>"$copydir.lock"
	    if ! flock -n 9; then
		stat_busy "Locking chroot copy '$copy'"
		flock 9
		stat_done
	    fi

	    if [[ "$(stat -f -c %T "${copy}")" == btrfs ]]; then
		{ type -P btrfs && btrfs subvolume delete "${copy}"; } &>/dev/null
	    fi
	    sudo rm -rf --one-file-system "${copy}"
	done
	exec 9>&-

	sudo rm -rf --one-file-system "${workdir}"
	sudo mkdir -p "${workdir}"
	sudo setarch "${arch}" mkmanjaroroot \
	    -C "/usr/share/devtools/pacman-${pacman_conf_arch}.conf" \
	    -M "/usr/share/devtools/makepkg-${arch}.conf" \
	    -b "${branch}" \
	    -a "${arch}" \
	    "${workdir}/root" \
	    "${base_packages[@]}" || abort
    else
	sudo setarch ${arch} mkmanjaroroot \
	    -u \
	    -C "/usr/share/devtools/pacman-${pacman_conf_arch}.conf" \
	    -M "/usr/share/devtools/makepkg-${arch}.conf" \
	    -b "${branch}" \
	    -a "${arch}" \
	    "${workdir}/root" || abort
    fi

    msg "Created chroot for [${branch}] (${arch})"
}

# $1: name
chroot_pkg(){
    for pkg in $(cat ${profiledir}/$1); do
	cd $pkg
	if [[ $pkg == 'eudev' ]];then
	    sudo makechrootpkg -b ${branch} -r ${workdir}  || break
	    local blacklist=('libsystemd')
	    local user=$(ls ${workdir} | cut -d' ' -f1 | grep -v root | grep -v lock)
	    sudo pacman -Rdd ${blacklist[@]} -r ${workdir}/${user} --noconfirm
	else
	    sudo makechrootpkg -b ${branch} -r ${workdir}  || break
	fi
	cd ..
    done
}

chroot_build(){
    cd $1
    if ${clean_first}; then
	sudo ${branch}-${arch}-build -c -r ${chroots}
    else
	sudo ${branch}-${arch}-build -r ${chroots}
    fi
    cd ..
}

mv_profile_pkgs(){
    for pkg in $(cat ${profiledir}/$1); do
	mv_pkg $pkg
    done
}

mv_pkg(){
    cd ${rundir}
    cd $1
    msg2 "Moving $1 to ${pkgdir}"
    mv -v *.pkg.tar.xz ${pkgdir}
    cd ..
}

mv_pkgs(){
    if [[ ${arch} == "i686" ]];then
	mkdir -p ${pkgdir}i686
	pkgdir=${pkgdir}/i686
    else
	mkdir -p ${pkgdir}/x86_64
	pkgdir=${pkgdir}/x86_64
    fi
    eval "case $1 in
	$profiles)
	    mv_profile_pkgs $1
	;;
	*)
	    mv_pkg $1
	;;
    esac"
}

sign_pkgs(){
    cd ${pkgdir}
    signpkgs
    cd ..
}

git_clean(){
    git clean -dfx
}

get_profiles(){
    local prof=
    for p in $(ls ${profiledir});do
	prof=${prof:-}${prof:+|}$p
    done
    echo $prof
}

run(){
    eval "case ${profile} in
	$profiles)
	    chroot_init
	    msg3 'Start building profile:' "${profile}"
	    chroot_pkg ${profile}
	    msg3 'Finished building profile:' "${profile}"
	;;
	*)
	    chroot_build ${profile}
	;;
    esac"
}

#################################MAIN##########################################

export LANG=C
export LC_MESSAGES=C

unset ALL_OFF BOLD BLUE GREEN RED YELLOW
if [[ -t 2 ]]; then
	# prefer terminal safe colored and bold text when tput is supported
	if tput setaf 0 &>/dev/null; then
		ALL_OFF="$(tput sgr0)"
		BOLD="$(tput bold)"
		BLUE="${BOLD}$(tput setaf 4)"
		GREEN="${BOLD}$(tput setaf 2)"
		RED="${BOLD}$(tput setaf 1)"
		YELLOW="${BOLD}$(tput setaf 3)"
	else
		ALL_OFF="\e[1;0m"
		BOLD="\e[1;1m"
		BLUE="${BOLD}\e[1;34m"
		GREEN="${BOLD}\e[1;32m"
		RED="${BOLD}\e[1;31m"
		YELLOW="${BOLD}\e[1;33m"
	fi
fi
readonly ALL_OFF BOLD BLUE GREEN RED YELLOW

pacman_conf_arch='default'
arch=$(uname -m)
branch="unstable"
base_packages=(base-devel)

if [ "$arch" == 'multilib' ]; then
	pacman_conf_arch='multilib'
	arch='x86_64'
	branch="${branch}-multilib"
	base_packages+=(multilib-devel)
fi

chroots=/srv/manjarobuild
clean_first=false
sign=0
profile=all # dynamic profiles as defined in profiledir(must not be a dir name in tree to work)
rundir=$(pwd)
profiledir=${rundir}/profiles
profiles=$(get_profiles)

pkgdir=${rundir}/packages

usage() {
    echo "Usage: $0 [options]"
    echo '    -h                   This help'
    echo '    -a <arch>            Set arch'
    echo '    -b <branch>          Set branch'
    echo '    -c                   Recreate the chroot before building'
    echo '    -p <profile>         Set profile or pkg'
    echo '    -r <dir>             Create chroots in this directory'
    echo '    -s                   Sign packages'
    echo ''
    echo ''
    exit 1
}

while getopts 'r:a:b:p:hcs' arg; do
    case "${arg}" in
	a) arch="$OPTARG" ;;
	b) branch="$OPTARG" ;;
	c) clean_first=true ;;
	p) profile="$OPTARG" ;;
	r) chroots="$OPTARG" ;;
	s) sign=1 ;;
	*) usage ;;
    esac
done

workdir=${chroots}/${branch}-${arch}

git_clean

run $@

mv_pkgs ${profile}

if (( ${sign} ));then
    sign_pkgs
fi
