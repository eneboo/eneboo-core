#!/bin/bash

./build.sh -prefix "$(pwd)/eneboo-build-linux64-dba/" -platform "linux-g++-64" -sqllog -dbadmin && ./clean_build.sh "eneboo-build-linux64-dba"