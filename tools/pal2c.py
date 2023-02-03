#!/usr/bin/env python3

# scrape.py - Public domain

import os
import sys
from pathlib import Path
from typing import TextIO, BinaryIO


def convertPal(pal: bytes, out: TextIO):
	num = len(pal) // 3
	out.writelines([f"uint32_t palette[{num}] =\n", "{\n"])
	for i in range(num):
		base = i * 3
		out.writelines([
			"\tMAKE_RGB(",
			", ".join(f"0x{j:02X}" for j in pal[base:base + 3]),
			")," if i < num - 1 else ")",
			"\n"])
	out.write("};\n")


def readLbmPal(f: BinaryIO, out: TextIO):
	if f.read(4) != b"FORM":
		exit("No IFF FORM chunk, not an LBM file")
	formLen = int.from_bytes(f.read(4), byteorder="big", signed=False)
	if formLen < 44:
		exit("Too small to be a real LBM file, truncated?")
	if f.read(4) not in [b"ILBM", b"PBM "]:
		exit("Not an ILBM or PBM file")
	while True:
		chunkId = f.read(4)
		chunkLen = int.from_bytes(f.read(4), byteorder="big", signed=False)
		if f.tell() + chunkLen >= formLen + 8:
			exit("EOF reached, no CMAP found")
		if chunkId == b"CMAP":
			convertPal(f.read(chunkLen), out)
			break
		else:
			f.seek(chunkLen + (chunkLen & 0x1), os.SEEK_CUR)


def main():
	outPath = None
	if len(sys.argv) == 3:
		outPath = Path(sys.argv[2])
	elif len(sys.argv) != 2:
		exit(f"Usage: {sys.argv[0]} <image.LBM> [out.txt]")

	inPath = Path(sys.argv[1])
	if outPath is not None and inPath.resolve() == outPath.resolve():
		exit("Woah there! Tried to write to the input file, stopping before things get spicy")

	try:
		with inPath.open("rb") as fin:
			if outPath is None:
				readLbmPal(fin, sys.stdout)
			else:
				with outPath.open("w") as fout:
					readLbmPal(fin, fout)
	except OSError as e:
		exit(f"Failed to open \"{e.filename}\": {e.strerror}")


if __name__ == "__main__":
	main()
