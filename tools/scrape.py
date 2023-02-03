#!/usr/bin/env python3

# scrape.py - (C) 2023 a dinosaur (zlib)

import json
import re
from pathlib import Path
from urllib.parse import urlparse, urljoin
from urllib.request import urlopen, Request as URLRequest
from urllib.error import URLError
from typing import TextIO
from argparse import ArgumentParser

import convert


def fetch(url: str, name: str|None = None) -> bool:
	path = Path(urlparse(url).path)
	if name is None:
		name = path.name
	path = Path(name)
	if path.exists():
		return True
	try:
		r = urlopen(URLRequest(url, data=None))
		p = r.read()
		if p.startswith(b"File does not exist"):
			raise URLError("File does not exist")
		with path.open("wb") as f:
			f.write(p)
		print(f"Fetched {url}")
		return True
	except URLError as e:
		print(f"Failed to fetch \"{name}\": {e}")
		return False


class Scene:
	def __init__(self,
			title: str|None=None, sound: str|None=None, maxVolume: float|None=None,
			month: str|None=None, script: str|None=None):
		self.title = title
		self.sound = sound
		self.maxVolume = maxVolume
		self.month = month
		self.script = script


def loadScenes(f: TextIO) -> dict[str, Scene]:
	s = f.read()

	# Remove js function wrapper (if one exists)
	s = re.match(r"var\s*?scenes\s*?=\s*?(\[[\s\S]*]);", s, re.MULTILINE)[1]
	# Remove multiline comment blocks
	s = re.sub(r"/\*\s*", "", s)
	s = re.sub(r"\s*\*/", "", s)
	# Remove single line comments
	s = re.sub(r"//.*$", "", s, flags=re.MULTILINE)
	# Quote properties
	s = re.sub(r"(\w*):", "\"\\1\":", s)
	# Replace single quoted values with double quotes
	s = re.sub(r"'(.*?)'", "\"\\1\"", s)
	# Remove trailing comma in objects
	s = re.sub(r",(\s*)}", r"\1}", s)

	j = json.loads(s)
	def parseScenes():
		for obj in j:
			name = obj.get("name")
			if name is not None:
				yield name, Scene(
					obj.get("title"), obj.get("sound"), obj.get("maxVolume"),
					obj.get("month"), obj.get("scpt"))
	return {name: scene for name, scene in parseScenes()}


def parseUrl(base: str|None, resource: str):
	if base is not None and not base.endswith("/"):
		base += "/"
	if resource is None:
		return None
	res = urlparse(resource)
	if not bool(res.netloc) and base is not None:
		return urljoin(base, res.path)
	return resource


def scrape(outDir: Path, scenesJsLoc: str, imgLoc: str, audioLoc: str | None, extraImg: list[str] | None = None):
	scenesJs = outDir.joinpath("scenes.js")
	if not fetch(scenesJsLoc, str(scenesJs)):
		exit(1)
	with scenesJs.open("r") as f:
		scenes = loadScenes(f)

	if extraImg is not None:
		for i in extraImg:
			if i not in scenes.keys():
				scenes[i] = Scene()

	# fetch raw LBM.json files
	jsonDir = outDir.joinpath("srcjson")
	jsDir = outDir.joinpath("srcjs")
	sounds = {}
	for i in scenes.items():
		if i[1].month is not None:
			resouce = f"scene.php?file={i[0]}&month={i[1].month}&script={i[1].script}&callback=CanvasCycle.initScene"
			urlBase = scenesJsLoc.removesuffix("scenes.js") + resouce
			jsDir.mkdir(exist_ok=True)
			fetch(urlBase, str(jsDir.joinpath(f"{i[0]}.LBM.js")))
		else:
			resouce = f"{i[0]}.LBM.json"
			urlBase = parseUrl(imgLoc, resouce)
			jsonDir.mkdir(exist_ok=True)
			fetch(urlBase, str(jsonDir.joinpath(f"{i[0]}.LBM.json")))
		if i[1] not in sounds and i[1].sound is not None:
			sounds[i[1].sound] = sounds.get(i[1].sound, 0) + 1

	# fetch all (ogg) audio files
	if audioLoc is not None:
		oggDir = outDir.joinpath("audio")
		oggDir.mkdir(exist_ok=True)
		for i in sounds.keys():
			fetch(parseUrl(audioLoc, f"{i}.ogg"), str(oggDir.joinpath(f"{i}.ogg")))

	# convert LBM.js back into actual LBM files
	lbmDir = outDir.joinpath("conv")
	lbmDir.mkdir(exist_ok=True)
	for i in scenes.items():
		js = jsonDir.joinpath(f"{i[0]}.LBM.json")
		if i[1].sound is not None:
			volume = i[1].maxVolume
			if volume is None:
				volume = 0.5
			volume = int(256 * volume) - 1
			oggv = oggDir.joinpath(f"{i[1].sound}.ogg").open("rb") if audioLoc is not None and sounds.get(i[1].sound, 0) == 1 else None
			convert.convert(js, lbmDir, i[1].title, i[1].sound, volume, oggv)
		else:
			convert.convert(js, lbmDir, i[1].title)


def main():
	parser = ArgumentParser()
	parser.add_argument("-base", type=str, required=False)
	parser.add_argument("-scenes", type=str, required=True)
	parser.add_argument("-images", type=str, required=True)
	parser.add_argument("-audio", type=str, required=False)
	parser.add_argument("-out", type=Path, required=True)
	parser.add_argument("-img", action="append", type=str, required=False, default=[])
	args = parser.parse_args()

	scenesJs = parseUrl(args.base, args.scenes)
	imgLoc   = parseUrl(args.base, args.images)
	audioLoc = parseUrl(args.base, args.audio)
	args.out.mkdir(exist_ok=True)
	scrape(args.out, scenesJs, imgLoc, audioLoc, [*args.img])


if __name__ == "__main__":
	main()
