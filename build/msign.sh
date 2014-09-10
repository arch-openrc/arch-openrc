#!/bin/bash
# msign.sh : sign openrc and eudev packages after building
# If you sign packages as root then this command can be run with sudo or as su

# Keep track of the base directory
cwd=`pwd`

## eudev
cd ../eudev
for pkg in $(cat build-list); do cd $pkg && signpkgs && cd ..; done

## openrc
cd $cwd # Back to the base directory
cd ../openrc
for pkg in $(cat build-list); do cd $pkg && signpkgs && cd ..; done

