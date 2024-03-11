/* standard.rs - Standardised ILBM chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fs, io};
use std::io::{Error, Read};
use super::{ChunkReaders, IFFChunk};
use crate::fsext::FileExt;


// https://1fish2.github.io/IFF/IFF%20docs%20with%20Commodore%20revisions/ILBM.pdf
pub(crate) struct ColourMap { pub(crate) entries: Vec<[u8; 3]> }

impl IFFChunk for ColourMap
{
	fn read(file: &mut fs::File, size: usize) -> io::Result<(ChunkReaders, usize)>
	{
		let mut palette = vec![[0u8; 3]; size / 3];
		let mut bytesRead = 0;
		for i in &mut palette { bytesRead += file.read(i)?; }
		Ok((ChunkReaders::ColourMap(Self { entries: palette }), bytesRead))
	}

	const ID: [u8; 4] = *b"CMAP";
	const SIZE: u32 = 0;
}


pub(crate) struct Grab { pub(crate) point: (i16, i16) }

impl IFFChunk for Grab
{
	fn read(file: &mut fs::File, _size: usize) -> io::Result<(ChunkReaders, usize)>
	{
		Ok((ChunkReaders::Grab(Self { point: (file.read_i16be()?, file.read_i16be()?) }),
			Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"GRAB";
	const SIZE: u32 = 4;
}


pub(crate) struct Body
{
	pub(crate) len: usize
}

impl IFFChunk for Body
{
	fn read(_file: &mut fs::File, size: usize) -> Result<(ChunkReaders, usize), Error>
	{
		return Ok((ChunkReaders::Body(Self { len: size - 4 }), Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"BODY";
	const SIZE: u32 = 0;
}
