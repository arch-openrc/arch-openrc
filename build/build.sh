#!/bin/bash
## build.sh: all the steps for building openrc and eudev packages

# Clean
./prebuild.sh

# Build
sudo ./mbuild.sh
#sudo mbuild.sh unstable i686
#sudo mbuild.sh testing
#sudo mbuild.sh testing i686

# Sign
./msign.sh
#sudo ./msign.sh

# Copy and post install
./postbuild.sh
