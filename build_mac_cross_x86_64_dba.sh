#!/bin/bash
export CROSS='x86_64-apple-darwin17-'
export PATH=$PATH:/opt/osxcross/bin

./build.sh -platform "macx-clang-cross" -dbadmin && ./clean_build.sh "eneboo-build-mac_x86_64-dba"
