#!/bin/bash
## prebuild.sh: pre install steps before building openrc and eudev packages

# Keep track of the base directory
cwd=`pwd`

cd ..
git clean -dfx  # Remove untracked directories and files
