#!/bin/sh
set -e

EMSDK="$HOME/Git/emsdk"
TARGET="colourcyclinginthehousetonight"

SPDXID="Zlib"
LICENSE="../COPYING"
URL="https://github.com/ScrelliCopter/colourcyclinginthehousetonight"

SOURCE=".."
DESTDIR="out"
BUILDDIR="build"

. "$EMSDK/emsdk_env.sh"

emcmake cmake -GNinja "$SOURCE" -B "$BUILDDIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILDDIR"
mkdir -p "$DESTDIR"
cp "$BUILDDIR/$TARGET.wasm" "$DESTDIR/$TARGET.wasm"
cp "$BUILDDIR/$TARGET.data" "$DESTDIR/$TARGET.data"
(
	echo "// @license magnet:?xt=urn:btih:922bd98043fa3daf4f9417e3e8fec8406b1961a3&dn=zlib.txt $SPDXID"
	echo "/**"
	echo " * @source: $URL"
	echo " * @SPDX-License-Identifier: $SPDXID"
	echo " *"
	sed -e 's/^/ * /' "$LICENSE"
	echo " */"
	cat "$BUILDDIR/$TARGET.js"
	echo "// @license-end"
) > "$DESTDIR/$TARGET.js"
cp index.html "$DESTDIR/index.html"
