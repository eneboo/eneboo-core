#!/bin/bash

./build.sh -prefix "$(pwd)/eneboo-build-linux64-quick" -platform "linux-g++-64" -flfcgi -quick -gdb && ./clean_build.sh "eneboo-build-linux64-quick"
