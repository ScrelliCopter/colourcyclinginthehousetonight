#!/usr/bin/env python3

from collections import namedtuple
from urllib.request import urlopen
from urllib.error import URLError


def main():
	File = namedtuple("File", ["repo", "rpath", "dstpath"])
	files = [
		File("nothings/stb", "stb_vorbis.c", "src/stb_vorbis.h"),
		File("hsluv/hsluv-c", "src/hsluv.h", "src/hsluv.h"),
		File("hsluv/hsluv-c", "src/hsluv.c", "src/hsluv.c")]

	for file in files:
		remote = f"https://raw.githubusercontent.com/{file.repo}/master/{file.rpath}"
		try:
			with urlopen(remote) as request, \
					open(file.dstpath, "w", newline="\n") as out:
				out.write(request.read().decode("UTF-8").replace("\r\n", "\n"))
		except URLError as e:
			exit(f"Failed to fetch \"{remote}\": {e}")


if __name__ == "__main__":
	main()
