#!/bin/bash
## pbuild.sh: post install steps after building openrc and eudev packages

# Keep track of the base directory
cwd=`pwd`

# Specify a directory for storing built packages
pkgdir="$cwd/packages"

# Copy built packages and their sig files to $pkgdir
## eudev
cd eudev
for pkg in $(cat build-list); do cd $pkg && cp *.pkg.tar.xz $pkgdir; cp *.pkg.tar.xz.sig $pkgdir; cd ..; done
## openrc
cd $cwd # Back to the base directory
cd openrc
for pkg in $(cat build-list); do cd $pkg && cp *.pkg.tar.xz $pkgdir; cp *.pkg.tar.xz.sig $pkgdir; cd ..; done

# Perform any other commands

