/* header.rs - Standard ILBM header "BMHD" chunk
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fmt, fs, io};
use super::{ChunkReaders, IFFChunk};
use crate::fsext::FileExt;


// https://1fish2.github.io/IFF/IFF%20docs%20with%20Commodore%20revisions/ILBM.pdf
pub(crate) struct LBMHeader
{
	pub(crate) size: (u16, u16),
	pub(crate) offset: (i16, i16),
	pub(crate) numPlanes: u8,
	pub(crate) masking: Mask,
	pub(crate) compression: Compression,
	pub(crate) pad1: u8,
	pub(crate) transparent: u16,
	pub(crate) aspect: (u8, u8),
	pub(crate) pageSize: (i16, i16)
}

impl IFFChunk for LBMHeader
{
	fn read(file: &mut fs::File, _size: usize) -> io::Result<(ChunkReaders, usize)>
	{
		Ok((ChunkReaders::LBMHeader(Self {
			size:        (file.read_u16be()?, file.read_u16be()?),
			offset:      (file.read_i16be()?, file.read_i16be()?),
			numPlanes:   file.read_u8()?,
			masking:     Mask(file.read_u8()?),
			compression: Compression(file.read_u8()?),
			pad1:        file.read_u8()?,
			transparent: file.read_u16be()?,
			aspect:      (file.read_u8()?, file.read_u8()?),
			pageSize:    (file.read_i16be()?, file.read_i16be()?)
		}), Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"BMHD";
	const SIZE: u32 = 20;
}


#[derive(PartialEq, Eq)]
pub(crate) struct Mask(u8);
impl Mask
{
	pub(crate) const NONE: Mask            = Mask(0);
	pub(crate) const HAS_MASK: Mask        = Mask(1);
	pub(crate) const HAS_TRANSPARENT: Mask = Mask(2);
	pub(crate) const LASSO: Mask           = Mask(3);
}

impl Default for Mask { fn default() -> Self { Mask::NONE } }

impl fmt::Display for Mask
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", match *self
		{
			Self::NONE            => "None",
			Self::HAS_MASK        => "Masked",
			Self::HAS_TRANSPARENT => "Transparency",
			Self::LASSO           => "Lasso",
			_ => { panic!("Shouldn't happen") }
		})
	}
}


#[derive(PartialEq, Eq)]
pub(crate) struct Compression(u8);
impl Compression
{
	pub(crate) const NONE: Compression         = Compression(0);
	pub(crate) const BYTE_RUN1: Compression    = Compression(1);
	pub(crate) const VERTICAL_RLE: Compression = Compression(2);
}

impl Default for Compression { fn default() -> Self { Compression::NONE } }

impl fmt::Display for Compression
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", match *self
		{
			Self::NONE         => "Uncompressed",
			Self::BYTE_RUN1    => "RLE (Unpacker)",
			Self::VERTICAL_RLE => "Atari (VDAT)",
			_ => { panic!("Shouldn't happen") }
		})
	}
}
