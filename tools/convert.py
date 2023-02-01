#!/usr/bin/env python3

import sys
import re
from pathlib import Path
import json
import struct
from enum import Enum
from collections import namedtuple
from dataclasses import dataclass
from typing import BinaryIO


class Format(Enum):
	ILBM = b"ILBM"
	PBM  = b"PBM "


class IffChunk(Enum):
	IFF_FORM      = b"FORM"
	BITMAP_HEADER = b"BMHD"
	COLOUR_MAP    = b"CMAP"
	EXT_CLRRANGE  = b"CRNG"
	BODY          = b"BODY"
	CUST_SCENENFO = b"SNFO"
	CUST_OGGVORB  = b"OGGV"
	CUST_SPANS    = b"SPAN"


class Mask(Enum):
	NONE                   = 0
	HAS_MASK               = 1
	HAS_TRANSPARENT_COLOUR = 2
	LASSO                  = 3


class Compression(Enum):
	NONE      = 0
	BYTE_RUN1 = 1


class RangeFlag(Enum):
	NONE    = 0x0
	ACTIVE  = 0x1
	REVERSE = 0x2


#CycleRange = namedtuple("CycleRange", "rate flags low high")
@dataclass(frozen=True)
class CycleRange:
	rate: int
	flags: RangeFlag
	low: int
	high: int



def makeChunk(chunkId: IffChunk, *content: bytes) -> bytes:
	assert len(chunkId.value) == 4, f"Value of {chunkId} is not a fourcc"
	lenChunk = sum(len(i) for i in content)
	lenBytes = lenChunk.to_bytes(4, "big", signed=False)
	if lenChunk & 0x1 == 0x1:  # "it implies that the bit should be set to 1" -idiot cat, 2023
		# Pad non-even chunks to word boundaries
		return chunkId.value + lenBytes + b"".join(content) + b"\0"
	else:
		return chunkId.value + lenBytes + b"".join(content)


def makeHeader(size: (int, int)) -> bytes:
	ofs = (0, 0)
	nPlanes = 8
	masking = Mask.NONE
	compression = Compression.BYTE_RUN1
	transClr = 0
	aspect = (1, 1)
	pageWidth = size

	bmhd = struct.pack(">HHhhBBBBHBBhh",
		*size,              # BEUINT16, BEUINT16 = widght & height
		*ofs,               # BEINT16, BEINT16   = x & y origin
		nPlanes,            # UINT8              = source bitplanes
		masking.value,      # UINT8              = masking technique
		compression.value,  # UINT8              = compression method
		0,                  # UINT8              = pad1 (reserved)
		transClr,           # BEUINT16           = transparent colour
		*aspect,            # BEUINT8, BEUINT8   = aspect ratio (x:y)
		*pageWidth)         # BEINT16, BEINT16   = page width & height
	return makeChunk(IffChunk.BITMAP_HEADER, bmhd)


def makeColourRange(rate: int, flags: RangeFlag, range: (int, int)) -> bytes:
	crng = struct.pack(">hhhBB",
		0,            # BEINT16 = pad1 (reserved)
		rate,         # BEINT16 = rate
		flags.value,  # BEINT16 = flags
		*range)       # UINT8, UINT8 = low, high
	return makeChunk(IffChunk.EXT_CLRRANGE, crng)


def byteRun1Compress(data: bytes) -> bytes:
	def rleLiteral(b: list[int]) -> bytes:
		# [0..127, literal byte(s) n+1 to copy, ..]
		return (len(b) - 1).to_bytes(1, byteorder="big", signed=True) + bytes(b)

	def rleRun(v: int, num: int) -> bytes:
		# [-1..-127, next byte to replicate -n+1 times]
		return (-num + 1).to_bytes(1, byteorder="big", signed=True) + v.to_bytes(1, byteorder="big", signed=False)

	def rle():
		literal = []
		nextUniq = 0
		run = False
		for i, b in enumerate(data):
			if run and i < nextUniq:
				continue
			run = False
			if i >= nextUniq:
				# Find the index of the next unique byte (up to a run length of 128 at maximum)
				nextUniq = next((i + ii for ii, bb in enumerate(data[i:]) if bb != b or ii == 128), len(data))

			runLen = nextUniq - i
			# Only encode runs of 2 bytes if preceeded by a RLE packet (runs of 3 or more are always encoded as RLE)
			# Efficiency could be better if this detected a copy packet after runs of 2 and folded the pair into the latter
			if runLen > (2 if len(literal) else 1):
				if len(literal):
					# Commit current literal (copy) buffer
					yield rleLiteral(literal)
					literal.clear()
				yield rleRun(b, runLen)
				run = True
			else:
				literal.append(b)
				# Literal packets max out at 128 bytes, commit the current buffer if we're about to go over
				if len(literal) == 128:
					yield rleLiteral(literal)
					literal.clear()
		if len(literal):
			yield rleLiteral(literal)

	return b"".join(rle())


def imgCompress(b: bytes, stride: int) -> bytes:
	return b"".join(byteRun1Compress(b[i * stride:i * stride + stride]) for i in range(len(b) // stride))


def makeCustomSceneInfo(title: str|None, audio: str|None, volume: int):
	bTitle = title.encode("ASCII") if title is not None else b""
	bAudio = audio.encode("UTF-8") if audio is not None else b""
	assert len(bTitle) < 256, "Title can't be longer than 255 bytes"
	assert len(bAudio) < 256, "Audio can't be longer than 255 bytes"
	return makeChunk(IffChunk.CUST_SCENENFO,
		len(bTitle).to_bytes(1, byteorder="big", signed=False), bTitle,
		len(bAudio).to_bytes(1, byteorder="big", signed=False), bAudio,
		volume.to_bytes(1, byteorder="big", signed=False))


def computeSpans(pixRows: list[bytes], crng: list[CycleRange]) -> bytes:
	Span = namedtuple("Span", "left right inLeft inRight")
	emptySpan = Span(-1, -1, -1, -1)

	def getSpan(row: bytes) -> Span|None:
		def isCycling(p: int) -> bool:
			return next((True for i in crng if i.rate != 0 and i.high > i.low and i.low <= p <= i.high), False)

		def findInner(inRow: bytes) -> tuple[int, int]|None:
			tmpL = 0
			inner = (-1, -1)
			inHole = False
			for i, p in enumerate(inRow):
				if isCycling(p):
					inLen = (i - 1) - tmpL
					if inLen > inner[1] - inner[0]:
						inner = (tmpL, tmpL + inLen)
					tmpL = i
					inHole = False
				elif not inHole:
					tmpL = i
					inHole = True
			return inner if inner[0] >= 0 else None

		try:
			left = next(i for i, p in enumerate(row) if isCycling(p))
			right = next(len(row) - 1 - i for i, p in enumerate(reversed(row)) if isCycling(p))
		except StopIteration:
			return emptySpan
		if right < left:
			return emptySpan

		if right - left > 1 and (inner := findInner(row[left:right + 1])) is not None:
			return Span(left, right, left + inner[0], left + inner[1])
		else:
			return Span(left, right, -1, -1)

	spans = []
	startOfs = None
	for i, row in enumerate(pixRows):
		if startOfs is None:
			span = getSpan(row)
			if span is not emptySpan:
				startOfs = i
				spans.append(span)
		else:
			span = getSpan(row)
			if span is not emptySpan:
				for _ in range(startOfs + len(spans), i):
					spans.append(emptySpan)
				spans.append(span)

	out = bytearray(struct.pack(">HH", 0 if startOfs is None else startOfs, len(spans)))
	for span in spans:
		out += span.left.to_bytes(2, "big", signed=True)
		if span.left < 0:
			continue
		out += span.right.to_bytes(2, "big", signed=True)
		out += span.inLeft.to_bytes(2, "big", signed=True)
		if span[2] < 0:
			continue
		out += span.inRight.to_bytes(2, "big", signed=True)
	return bytes(out)


def makeCustomSpans(pix: bytes, stride: int, crng: list[CycleRange]) -> bytes:
	return makeChunk(IffChunk.CUST_SPANS,
		computeSpans([pix[i * stride:i * stride + stride] for i in range(len(pix) // stride)], crng))


remove_js = re.compile(r"\((\{[\s\S]*})", re.MULTILINE)
match_single = re.compile(r"'(.*?)'")
quote_properties = re.compile(r"(\w*):")


def convert(inPath: Path, outDir: Path, title: str|None=None, audio: str|None=None, volume: int|None=None, oggv: BinaryIO|None=None):
	with inPath.open("r") as f:
		s = f.read()
	if inPath.name.endswith(".js"):
		m = remove_js.search(s)
		if m and len(m.groups()):
			s = m[1]
	s = match_single.sub("\"\\1\"", s)
	s = quote_properties.sub("\"\\1\":", s)
	j = json.loads(s)

	pix = bytes(j["pixels"])
	size = (j["width"], j["height"])
	flag = lambda reverse: RangeFlag.REVERSE if reverse == 2 else RangeFlag.NONE
	crng = [CycleRange(i["rate"], flag(i["reverse"]), i["low"], i["high"]) for i in j["cycles"]]

	with outDir.joinpath(j["filename"]).open("wb") as out:
		chunks = []
		chunks.append(makeHeader(size))
		chunks.append(makeChunk(IffChunk.COLOUR_MAP, b"".join(bytes(x) for x in j["colors"])))
		for i in crng:
			chunks.append(makeColourRange(i.rate, i.flags, (i.low, i.high)))
		if title is not None or audio is not None:
			if volume is None:
				volume = 127 if audio is not None else 0
			chunks.append(makeCustomSceneInfo(title, audio, volume))
		chunks.append(makeCustomSpans(pix, size[0], crng))
		chunks.append(makeChunk(IffChunk.BODY, imgCompress(pix, size[0])))
		if oggv is not None:
			chunks.append(makeChunk(IffChunk.CUST_OGGVORB, oggv.read()))
		out.write(makeChunk(IffChunk.IFF_FORM, Format.PBM.value, *chunks))


def main():
	if len(sys.argv) == 3:
		convert(Path(sys.argv[1]), Path(sys.argv[2]))
	elif len(sys.argv) == 1:
		outDir = Path("conv")
		outDir.mkdir(exist_ok=True)
		for js in Path("srcjs").glob("*.LBM.js"):
			print(js)
			convert(js, outDir)
	else:
		exit("Incorrect argument number")


if __name__ == "__main__":
	main()
