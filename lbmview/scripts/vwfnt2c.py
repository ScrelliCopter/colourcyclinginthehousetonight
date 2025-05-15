#!/usr/bin/env python3

# vwfnt2c.py - (C) 2023, 2025 a dinosaur (zlib)

from pathlib import Path
from typing import TextIO, Iterable, Tuple
import xml.etree.ElementTree as ET
from typing import NamedTuple
from PIL import Image


Kerning = dict[str | None, dict[str | None, int]]


class Spec(NamedTuple):
	image: Image.Image
	charmap: list[str]
	space: int
	kerning: Kerning


def getint(e: ET.Element, attrib: str, default: None | int = None) -> int | None:
	value = e.get(attrib)
	return int(value) if value else default


def mergekern(unopt: Kerning) -> Kerning:
	merge: Kerning = dict()
	for i in unopt.items():
		if key := next((j[0] for j in merge.items() if j[0] is not None and j[1] == i[1]), None):
			merge[key + i[0]] = merge.pop(key)
		else:
			merge.update({i[0]: i[1]})
	kernings: Kerning = dict()
	for i in merge.items():
		if i[0] is None:
			kernings[None] = i[1]
			continue
		combined = {}
		for j in i[1].items():
			combined[j[1]] = combined.get(j[1], "") + j[0]
		kernings[i[0]] = dict((v, k) for k, v in combined.items())

	return kernings


def loadspec(file: TextIO, dir: Path) -> Spec:
	xroot = ET.parse(file).getroot()
	imgpath = Path(xroot.get("image"))
	if not imgpath.is_absolute():
		imgpath = dir.joinpath(imgpath)
	image = Image.open(imgpath).convert("RGB")
	spacewidth = getint(xroot, "space")
	if spacewidth is None:
		raise KeyError("Root tag must specify a 'space' width attribute")

	# Read charmap lines
	charmap: list[str] = list()
	for xline in xroot.find("charmap").iter("line"):
		charmap.append(xline.text)

	# Read kerning rules
	kernings: Kerning = dict()
	xkerning = xroot.find("kerning")
	defaultkern = getint(xkerning, "width", 0)
	if defaultkern is None:
		raise KeyError("kerning must specify a default 'width' attribute")
	kernings[None] = {None: defaultkern}
	for xoverride in xkerning.iter("override"):
		glyph = xoverride.get("glyph")
		if glyph is None:
			raise KeyError("override must have a 'glyph' attribute")
		elif len(glyph) != 1:
			raise ValueError("glyph refers to a single character")

		if (width := getint(xoverride, "width")) is not None:
			kernings.setdefault(glyph, {})[None] = width
		if (xprev := xoverride.find("prev")) is not None:
			width = getint(xprev, "width")
			if width is None:
				raise KeyError("prev must specify a 'width' attribute")
			for prevglyph in xprev.text:
				kernings.setdefault(glyph, {})[prevglyph] = width
		if (xnext := xoverride.find("next")) is not None:
			width = getint(xnext, "width")
			if width is None:
				raise KeyError("next must specify a 'width' attribute")
			for nextglyph in xnext.text:
				kernings.setdefault(nextglyph, {})[glyph] = width
	return Spec(image, charmap, spacewidth, mergekern(kernings))


def writekern(file: TextIO, spec: Spec, label: str = "GetKern"):
	def cchar(char: str, string: bool=False) -> str:
		if m := {
				"\a": "\\a", "\b": "\\b", "\f": "\\f", "\n": "\\n", "\r": "\\r",
				"\t": "\\t", "\v": "\\v", "\\": "\\\\", "\"": "\\\"" }.get(char):
			return m if string else f"'{m}'"
		elif (i := ord(char)) < 0x100:
			if i < 0x7F and char.isprintable():
				return char if string else f"'{char}'"
			else:
				return f"\\u{i:04X}" if string else f"0x{i:02X}"
		elif i < 0x10000:
			return f"\\u{i:04X}" if string else f"0x{i:04X}"
		else:
			return f"\\U{i:08X}" if string else f"0x{i:08X}"

	def explode(v: str, p: Iterable[str]) -> str:
		def ranges(matchstr: Iterable[str]) -> Iterable[Tuple[int, int]]:
			start = None
			last = None
			for c in sorted(matchstr):
				i = ord(c)
				if start is None:
					start = i
				elif i - last > 1:
					yield start, last
					start = i
				last = i
			yield start, last

		def conditions(var: str, matchstr: Iterable[str]) -> Iterable[str]:
			for r in ranges(matchstr):
				if r[0] == r[1]:
					yield f"{var} == {cchar(chr(r[0]))}"
				else:
					yield f"({var} >= {cchar(chr(r[0]))} && {var} <= {cchar(chr(r[1]))})"

		return " || ".join(conditions(v, p))

	def body(stuff: dict[str | None, int]):
		default = stuff.get(None)
		if default is not None and len(stuff) < 2:
			yield f" {{ return {default}; }}"
		else:
			yield "\n\t{"
			for i, p in enumerate(filter(lambda i: i[0], stuff.items())):
				if i == 0:
					yield "\n\t\tif"
					if len(stuff) > 1:
						yield "     "
				else:
					yield "\t\telse if"
				yield f" ({explode('p', p[0])}) {{ return {p[1]}; }}\n"
			if default is not None:
				yield f"\t\treturn {default};\n"
			yield "\t}"

	nonone = filter(lambda i: i[0], spec.kerning.items())
	file.write(f"int {label}(int c, int p)\n{{\n")
	file.write("\t" + "\n\telse ".join(f"if ({explode('c', p[0])}){''.join(body(p[1]))}" for p in nonone))
	file.write(f"\n\treturn {spec.kerning[None][None]};\n}}\n")


class Char(NamedTuple):
	char: str
	bits: bytes


def readchars(spec: Spec) -> list[Char]:
	getpix = lambda x, y: spec.image.getpixel((x, y))
	checkred = lambda p: p[0] > 0x7F > p[1] and p[2] < 0x7F
	checkblack = lambda p: p[0] < 0x7F and p[1] < 0x7F and p[2] < 0x7F

	chars: list[Char] = list()
	for linenum, line in enumerate(spec.charmap):
		ix = 0
		for char in line:
			while ix < spec.image.width and checkred(getpix(ix, linenum * 8)):
				ix += 1
			bitties = bytearray()
			for ix in range(ix, spec.image.width):
				if checkred(getpix(ix, linenum * 8)):
					ix += 1
					break
				bits = 0
				for iy in range(8):
					bits |= (1 if checkblack(getpix(ix, linenum * 8 + iy)) else 0) << iy
				bitties += bits.to_bytes(1, byteorder="big")
			chars.append(Char(char, bytes(bitties)))
	return chars


def writecharbits(file: TextIO, chars: list[Char], label: str = "glyphBitmaps"):
	file.write(f"const uint8_t {label}[{sum(len(char.bits) for char in chars)}] =\n{{\n")
	for i, char in enumerate(chars):
		if i != 0:
			file.write(",\n")
		file.write(f"/* '{char.char}' */  ")
		file.write(", ".join("0x{:02X}".format(byte) for byte in char.bits))
	file.write("\n};\n")


def writeasciiprintablelut(file: TextIO, chars: list[Char], label: str = "asciiLut"):
	i = 0
	asciimap: dict[int, tuple[int, int]] = dict()
	for char in chars:
		length = len(char.bits)
		if (charord := ord(char.char)) < 0x7F:
			asciimap[charord] = (i, length)
		i += length

	file.write(f"static const int {label}[256] =\n{{\n")
	for i in range(0x80):
		offset, size = -1, 0
		if (key := asciimap.get(i)) is not None:
			offset, size = key
		if (c := chr(i)).isprintable():
			file.write(f"/* '{c}' */")
		else:
			file.write(" " * 9)
		file.write(f"{offset: 4}, {size}")
		if i % 4 == 3:
			if i < 0x7F:
				file.write(",\n")
		else:
			file.write(", ")
	file.write("\n};\n")


def main():
	specpath = Path("vwbmf.xml")
	coutpath = Path("src/font.c")
	houtpath = Path("src/font.h")

	with specpath.open("r") as file:
		spec = loadspec(file, specpath.parent)

	with coutpath.open("w", newline="\n") as cout:
		chars = readchars(spec)

		pyname = Path(__file__).name
		getkern = "fontGetKerning"
		getglyph = "fontGetGlyph"
		glpyhbmp = "fontGlyphBitmaps"

		cout.writelines([
			f"/* Auto-generated by '{pyname}' */\n",
			f"#include \"{houtpath.name}\"\n",
			"\n"])
		writekern(cout, spec, label=getkern)
		cout.write("\n")
		writecharbits(cout, chars, label=glpyhbmp)
		cout.write("\n")
		writeasciiprintablelut(cout, chars)
		cout.write("\n")
		cout.writelines([
			f"int {getglyph}(int c, int* restrict offset, int* restrict size)\n",
			"{\n",
			"\tif (c < 0x80)\n",
			"\t{\n",
			"\t\tint i = c * 2;\n"
			"\t\tif (asciiLut[i] < 0)\n",
			"\t\t\treturn 0;\n",
			"\t\t(*offset) = asciiLut[i++];\n",
			"\t\t(*size)   = asciiLut[i];\n",
			"\t\treturn 1;\n",
			"\t}\n",
			"\treturn 0;\n",
			"}\n"])

	with houtpath.open("w", newline="\n") as hout:
		guard = pyname.upper().replace(".", "_")
		guard = f"{guard}_FONT"
		hout.writelines([
			f"/* Auto-generated by '{pyname}' */\n",
			"\n",
			f"#ifndef {guard}\n",
			f"#define {guard}\n",
			"\n",
			f"#define FONT_SPACE_WIDTH {spec.space}\n",
			"\n",
			f"extern int {getkern}(int c, int p);\n",
			f"extern int {getglyph}(int c, int* restrict offset, int* restrict size);\n",
			"\n",
			"#include <stdint.h>\n",
			f"extern const uint8_t {glpyhbmp}[{sum(len(char.bits) for char in chars)}];\n",
			"\n",
			f"#endif//{guard}\n"])


if __name__ == "__main__":
	main()
