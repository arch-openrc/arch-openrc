#!/bin/bash

# switch to basic language
export LANG=C
export LC_MESSAGES=C

# do UID checking here so someone can at least get usage instructions
if [ "$EUID" != "0" ]; then
    echo "error: This script must be run as root."
    exit 1
fi

user=$(ls /var/lib/manjarobuild/unstable-$(uname -m) | cut -d' ' -f1 | grep -v root | grep -v lock)

echo "Build eudev"
date
cd eudev/libsystemd-eudevcompat
### boxit ???
unstable-$(uname -m)-build -c

makechrootpkg -n -r /var/lib/manjarobuild/unstable-$(uname -m)
pacman -Rdd libsystemd -r /var/lib/manjarobuild/unstable-$(uname -m)/$user --noconfirm
pacman -U *$(uname -m)*pkg*z -r /var/lib/manjarobuild/unstable-$(uname -m)/$user --noconfirm

cd ../eudev
makechrootpkg -n -r /var/lib/manjarobuild/unstable-$(uname -m)
pacman -U *$(uname -m)*pkg*z -r /var/lib/manjarobuild/unstable-$(uname -m)/$user --noconfirm
echo "Done building eudev"

echo "Build openrc"
date
cd ../../openrc
for pkg in $(cat build-list); do
	cd $pkg
	makechrootpkg -n -r /var/lib/manjarobuild/unstable-$(uname -m) || break && cd ..
done
date
echo "Done building openrc"

echo "Build eudev (additional packages)"
date
cd ../eudev
for pkg in $(cat build-list); do
	cd $pkg
	makechrootpkg -n -r /var/lib/manjarobuild/unstable-$(uname -m) || break && cd ..
done
date
echo "Done building eudev (additional packages)"
#shutdown -h now
