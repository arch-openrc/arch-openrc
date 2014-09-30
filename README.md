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
	-c                   Recreate the chroot before building
	-s                   Sign packages
	-n                   Run namcap on the package
	-u                   Update the working copy of the chroot before building
	-C                   Clean pkgbuilds dir
	-d                   Skip dep check -- makepkg_arg
