#!/bin/sh
set -e

if [ $# -ne 2 ]; then
	echo "Usage: $0 <input.js> <output.js>"
	exit 1
fi


SPDXID="Zlib"
URL="https://github.com/ScrelliCopter/colourcyclinginthehousetonight"
LICENSE="../../COPYING"

(
	echo "// @license magnet:?xt=urn:btih:922bd98043fa3daf4f9417e3e8fec8406b1961a3&dn=zlib.txt $SPDXID"
	echo "/**"
	echo " * @source: $URL"
	echo " * @SPDX-License-Identifier: $SPDXID"
	echo " *"
	sed -e 's/^/ * /' "$LICENSE"
	echo " */"
	cat "$1"
	echo "// @license-end"
) > "$2"
