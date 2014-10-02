#!/bin/bash

. ./lib_devtools

get_profiles(){
    local prof=
    for p in $(ls ${profiledir}/*.set);do
	local temp=${p##*/}
	prof=${prof:-}${prof:+|}${temp%.set}
    done
    echo $prof
}

get_user(){
    local user=$(ls ${WORKDIR} | cut -d' ' -f1 | grep -v root | grep -v lock)
    echo $user
}

chroot_create(){
    setarch "${arch}" \
	mkmanjaroroot \
	    -C "${pacman_conf}" -M "${makepkg_conf}" \
	    -a "${arch}" -b "${branch}" "${WORKDIR}/root" \
	    "${base_packages[*]}"
}

chroot_update(){
    msg "Updating chroot for [${branch}] (${arch})"
    msg2 "$(get_user)"
    mkmanjaroroot -b ${branch} -u ${WORKDIR}/$(get_user)
    msg "Updated chroot for [${branch}] (${arch})"
}

chroot_prepare(){
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
}

chroot_init(){
    if ${clean_first} || [[ ! -d "${WORKDIR}" ]]; then
	msg "Creating chroot for [${branch}] (${arch})..."
	chroot_prepare
	chroot_create || abort
    else
	chroot_update || abort
    fi
}

chroot_mkpkg(){
    exec setarch ${arch} \
	makechrootpkg ${mkchpkg_args[*]} \
		      -b ${branch} -r ${WORKDIR} \
		      -- "${makepkg_args[*]}"
}

chroot_build_set(){
    chroot_init
    msg "Start building profile: [${profile}]"
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
    msg "Finished building profile: [${profile}]"
}

chroot_build(){
    cd ${profile}
	chroot_init && chroot_mkpkg
    cd ..
}

run(){
    eval "case ${profile} in
	$profiles)
	    chroot_build_set
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
base_packages=base-devel

clean_first=true
update_first=false
namcap=true
sign=false
verbose=false

makepkg_args=()
mkchpkg_args=()

profiledir="$(pwd)/sets"
# profiledir="@sysconfdir@/sets"

profiles=$(get_profiles)

usage() {
    echo "Usage: $0 [options] -- makepkg_args"
    echo "    -p <profile>       Set profile or pkg [default ${profile}]"
    echo "    -a <arch>          Set arch  [default ${arch}]"
    echo "    -b <branch>        Set branch [default ${branch}]"
    echo "    -r <dir>           Chroots directory [default ${chroots}]"
    echo "    -c                 Create persistent chroot [default ${clean_first}]"
    echo '    -u                 Update the working copy of the chroot before building'
    echo "    -n                 Run namcap check [default ${namcap}]"
    echo '    -s                 Sign packages'
    echo '    -v                 Verbose'
    echo '    -h                 This help'
    echo ''
    echo ''
    exit 1
}

opts='p:a:b:r:cunsvh'

while getopts "${opts}" arg; do
    case "${arg}" in
	p) profile="$OPTARG" ;;
	a) arch="$OPTARG" ;;
	b) branch="$OPTARG" ;;
	r) chroots="$OPTARG" ;;
	c) clean_first=false ;;
	u) update_first=true; clean_first=false ;;
	n) namcap=false;;
	s) sign=true ;;
	v) verbose=true ;;
	h) usage ;;
    esac
done

#shift $((OPTIND-1))

makepkg_args+=("${*:$OPTIND}")

if ${clean_first};then
    mkchpkg_args+=(-c)
fi

if ${namcap};then
    mkchpkg_args+=(-n)
fi

if [[ "$arch" == 'multilib' ]]; then
    pacman_conf_arch='multilib'
    arch='x86_64'
    branch="${branch}-multilib"
    base_packages="${base_packages} multilib-devel"
fi

WORKDIR=${chroots}/${branch}-${arch}

pacman_conf="/usr/share/devtools/pacman-${pacman_conf_arch}.conf"
makepkg_conf="/usr/share/devtools/makepkg-${arch}.conf"

# pacman_conf="@pkgdatadir@/pacman-${pacman_conf_arch}.conf"
# makepkg_conf="@pkgdatadir@/makepkg-${arch}.conf"

if ${verbose};then
    msg "OPTARGS:"
    msg2 "arch: $arch"
    msg2 "branch: $branch"
    msg2 "profile: $profile"
    msg2 "chroots: $chroots"

    msg "SETS:"
    msg2 "profiles: $profiles"

    msg "PATHS:"
    msg2 "workdir: $WORKDIR"
    msg2 "profiledir: $profiledir"
    msg2 "pacman_conf_arch: ${pacman_conf_arch}"
    msg2 "pacman_conf: ${pacman_conf}"
    msg2 "makepkg_conf: $makepkg_conf"

    msg "PKG:"
    msg2 "base_packages: ${base_packages[*]}"

    msg "MAKEPKGARGS:"
    msg2 "makepkg_args: ${makepkg_args[*]}"

    msg "FLAGS:"
    msg2 "clean_first: $clean_first"
    msg2 "update_first: $update_first"
    msg2 "namcap: ${namcap}"
    msg2 "sign: $sign"
    msg2 "mkchpkg_args: ${mkchpkg_args[*]}"
fi

if [[ $EUID !=  0 ]]; then
    die 'This script must be run as root.'
else
    run $@
fi
