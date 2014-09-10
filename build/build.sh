#!/bin/bash
## build.sh: all the steps for building openrc and eudev packages

# Clean
echo "Performing pre-build tasks..."
./prebuild.sh

# Build
sudo ./mbuild.sh
#sudo ./mbuild.sh unstable i686
#sudo ./mbuild.sh testing
#sudo ./mbuild.sh testing i686

# Sign
echo "Signing packages..."
./msign.sh
#sudo ./msign.sh

# Copy and post install
echo "Performing post-build tasks..."
./postbuild.sh
