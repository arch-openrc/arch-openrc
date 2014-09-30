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
    printf "${YELLOW}==>${ALL_OFF}${BOLD} ${mesg}${ALL_OFF}${BOLD} ${mesg2}${ALL_OFF}\n" "$@" >&2
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
	    -C "${pacman_conf}" \
	    -M "${makepkg_conf}" \
	    -b "${branch}" \
	    -a "${arch}" \
	    "${workdir}/root" \
	    "${base_packages[@]}" || abort

	msg "Created chroot for [${branch}] (${arch}) $(stat_done)"
    else
	sudo setarch ${arch} mkmanjaroroot \
	    -C "${pacman_conf}" \
	    -M "${makepkg_conf}" \
	    -b "${branch}" \
	    -a "${arch}" \
	    "${workdir}/root" || abort
    fi
}

chroot_pkg(){
    local user=$(ls ${workdir} | cut -d' ' -f1 | grep -v root | grep -v lock)
    if ${update_first};then
	sudo mkmanjaroroot -u -b ${branch} ${workdir}/${user}
    fi
    for pkg in $(cat ${profiledir}/${profile}); do
	cd $pkg
	sudo makechrootpkg ${namcap_arg} -b ${branch} -r ${workdir} -- ${nodeps_arg} || break
	if [[ $pkg == 'eudev' ]];then
	    local blacklist=(libsystemd)
	    sudo pacman -Rdd ${blacklist[@]} -r ${workdir}/${user} --noconfirm
	fi
	cd ..
    done
}

chroot_build(){
    cd ${profile}
    if ${clean_first}; then
	sudo ${branch}-${arch}-build -c -r ${chroots}
    else
	sudo ${branch}-${arch}-build -r ${chroots}
    fi
    cd ..
}

cp_profile_pkgs(){
    for pkg in $(cat ${profiledir}/$1); do
	cp_pkg $pkg
    done
}

cp_pkg(){
    msg2 "Copying $1 to ${pkgdir}"
    cd $1
    cp -v *.pkg.tar.xz ${pkgdir}
    cd ..
}

cp_pkgs(){
    if ! [[ -d ${pkgdir}/${arch} ]];then
	mkdir -pv ${pkgdir}/${arch}
    fi
    pkgdir=${pkgdir}/${arch}
    eval "case $1 in
	$profiles)
	    cp_profile_pkgs $1
	;;
	*)
	    cp_pkg $1
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
	    msg3 'Start building profile:' '['"${profile}"']'
	    chroot_pkg ${profile}
	    msg3 'Finished building profile:' '['"${profile}"']'
	;;
	*)
	    chroot_build
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
profile=all # dynamic profiles as defined in profiledir(must not be a dir name in tree to work)

clean_first=false
sign=false
update_first=false
namcap=false
namcap_arg=
clean_build=false
nodeps=false
nodeps_arg=

rundir=$(pwd)
profiledir=${rundir}/profiles
profiles=$(get_profiles)
pkgdir=${rundir}/packages
pacman_conf=/usr/share/devtools/pacman-${pacman_conf_arch}.conf
makepkg_conf=/usr/share/devtools/makepkg-${arch}.conf

usage() {
    echo "Usage: $0 [options]"
    echo '    -h                   This help'
    echo '    -a <arch>            Set arch'
    echo '    -b <branch>          Set branch'
    echo '    -p <profile>         Set profile or pkg'
    echo '    -r <dir>             Create chroots in this directory'
    echo '    -c                   Recreate the chroot before building'
    echo '    -s                   Sign packages'
    echo '    -n                   Run namcap on the package'
    echo '    -u                   Update the working copy of the chroot before building'
    echo '    -C                   Clean pkgbuilds dir'
    echo '    -d                   Skip dep check -- makepkg_arg'
    echo ''
    echo ''
    exit 1
}

while getopts 'a:b:p:r:csnuhCd' arg; do
    case "${arg}" in
	a) arch="$OPTARG" ;;
	b) branch="$OPTARG" ;;
	p) profile="$OPTARG" ;;
	r) chroots="$OPTARG" ;;
	c) clean_first=true ;;
	s) sign=true ;;
	n) namcap=true ;;
	u) update_first=true ;;
	C) clean_build=true ;;
	d) nodeps=true ;;
	*) usage ;;
    esac
done

shift $((OPTIND-1))

workdir=${chroots}/${branch}-${arch}

if ${namcap};then
    namcap_arg=-n
fi

if ${nodeps};then
    nodeps_arg=-d
fi

msg2 "arch: $arch"
msg2 "branch: $branch"
msg2 "profile: $profile"
msg2 "chroots: $chroots"
msg2 "clean_first: $clean_first"
msg2 "sign: $sign"
msg2 "namcap: $namcap"
msg2 "namcap_arg: $namcap_arg"
msg2 "update_first: $update_first"
msg2 "namcap_arg: $namcap_arg"
msg2 "clean_build: $clean_build"
msg2 "nodeps: $nodeps"
msg2 "nodeps_arg: $nodeps_arg"


if ${clean_build};then
    git_clean
fi

run $@

cp_pkgs ${profile}

if ${sign};then
    sign_pkgs
fi
