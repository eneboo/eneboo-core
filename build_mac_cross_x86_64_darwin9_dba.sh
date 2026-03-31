#!/bin/bash
# Cross-compila eneboo para macOS x86_64 (darwin9 / 10.5 Leopard) desde Ubuntu 12.04
# Usa el toolchain GCC en /opt/mac con el prefijo x86_64-apple-darwin9-
# El binario resultante es x86_64 y corre bajo Rosetta 2 en Apple Silicon.
#
# Requisito: toolchain x86_64-apple-darwin9 en /opt/mac/bin
#            SDK en /opt/mac/SDKs/MacOSX10.5.sdk
export CROSS='x86_64-apple-darwin9-'
export PATH=$PATH:/opt/mac/bin

./build.sh -platform "macx-g++-cross-x86_64" -dbadmin && ./clean_build.sh "eneboo-build-mac_x86_64_darwin9-dba"
