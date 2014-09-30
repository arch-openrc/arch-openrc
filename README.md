OpenRC pkgbuilds
=========

###Groups:###

* openrc
* openrc-base
* openrc-desktop
* openrc-misc
* openrc-devel
* openrc-net
* eudev-base


###Build.sh###

    Usage: build.sh [options]
    -h                   This help
    -a <arch>            Set arch
    -b <branch>          Set branch
    -p <profile>         Set profile or pkg
    -r <dir>             Create chroots in this directory
    -c [default]         Recreate the chroot before building
    -s                   Sign packages
    -v                   Run namcap on the package
    -u                   Update the working copy of the chroot before building
    -w                   Clean pkgbuilds dir
    -d                   Skip dep check -- makepkg_arg
