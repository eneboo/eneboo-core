#!/bin/bash

VERSION=$(git describe --tags)
PROJECT="eneboo"
PVERSION="${PROJECT}-${VERSION}-quick"
SRC="src/qt"
test -e "$SRC" || { echo "No existe compilación para MacOSX ! (falta carpeta $SRC)"; exit 1; }

echo "Exportando compilación MacOSX x86_64 para $PVERSION . . . "

mv "$SRC" "$PVERSION"

rm -Rf "$PVERSION/lib/*.a"
rm -Rf "$PVERSION/lib/*.o"
rm -Rf "$PVERSION/lib/*.prl"
rm -Rf "$PVERSION/plugins/src"
rm -Rf "$PVERSION/mkspecs"
rm -Rf "$PVERSION/include"
rm -Rf "$PVERSION/templates"
rm -Rf "$PVERSION/config.tests"
rm -Rf "$PVERSION/qmake"
rm -Rf "$PVERSION/src"
rm -Rf "$PVERSION/tools"

tar cf "$PVERSION-macOSX_x86_64.tar" "$PVERSION"
bzip2 -9 "$PVERSION-macOSX_x86_64.tar"

mkdir "export/" 2>/dev/null

cp "$PVERSION-macOSX_x86_64.tar.bz2" "export/"
unlink "$PVERSION-macOSX_x86_64.tar.bz2"
rm -Rf "$PVERSION"


echo "Compilación exportada a: export/$PVERSION-macOSX_x86_64.tar.bz2"
