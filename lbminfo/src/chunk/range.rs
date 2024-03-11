/* range.rs - Non-standard colour cycling animation chunks written by Deluxe Paint
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fs, io};
use std::io::Read;
use super::{ChunkReaders, IFFChunk};
use crate::fsext::FileExt;
use crate::optionset::{optionset, optionset_custom_display};


// Standard Deluxe Paint colour cycling range chunk
// https://1fish2.github.io/IFF/IFF%20docs%20with%20Commodore%20revisions/ILBM.pdf
pub(crate) struct CycleRange
{
	pub(crate) pad1: i16,
	pub(crate) rate: i16,
	pub(crate) flags: RangeFlags,
	pub(crate) low: u8,
	pub(crate) high: u8
}

impl IFFChunk for CycleRange
{
	fn read(file: &mut fs::File, _size: usize) -> Result<(ChunkReaders, usize), io::Error>
	{
		Ok((ChunkReaders::CycleRange(Self {
			pad1: file.read_i16be()?,
			rate: file.read_i16be()?,
			flags: RangeFlags(file.read_i16be()?),
			low: file.read_u8()?,
			high: file.read_u8()?
		}), Self::SIZE as usize))
	}
	const ID: [u8; 4] = *b"CRNG";
	const SIZE: u32 = 8;
}


// Deluxe Paint IV "enhanced" colour cycle chunk
// https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#ILBM.DRNG
pub(crate) struct EnhancedColourCycle
{
	pub(crate) min: u8,
	pub(crate) max: u8,
	pub(crate) rate: i16,
	pub(crate) flags: RangeFlags,
	#[allow(dead_code)]
	pub(crate) numTrue: u8,
	#[allow(dead_code)]
	pub(crate) numRegs: u8,

	pub(crate) trueRange: Vec<EnhancedColour>,
	pub(crate) regsRange: Vec<EnhancedIndex>,

	pub(crate) fadeExt: Option<EnhancedColourCycleFadeExt>
}

#[derive(Default, Clone)]
pub(crate) struct EnhancedColour
{
	pub(crate) cell: u8,
	pub(crate) rgb: [u8; 3]
}

#[derive(Default, Clone)]
pub(crate) struct EnhancedIndex
{
	pub(crate) cell: u8,
	pub(crate) index: u8
}


// Undocumented Deluxe Paint V fade extension
// https://github.com/svanderburg/libilbm/blob/master/doc/additions/ILBM.DRNG.asc
pub(crate) struct EnhancedColourCycleFadeExt
{
	#[allow(dead_code)]
	pub(crate) numFade: u8,
	#[allow(dead_code)]
	pub(crate) fadePad: u8,

	pub(crate) fadeRange: Vec<EnhancedFade>
}

#[derive(Default, Clone)]
pub(crate) struct EnhancedFade
{
	pub(crate) cell: u8,
	pub(crate) fade: u8
}


impl IFFChunk for EnhancedColourCycle
{
	fn read(file: &mut fs::File, size: usize) -> io::Result<(ChunkReaders, usize)>
	{
		let min     = file.read_u8()?;
		let max     = file.read_u8()?;
		let rate    = file.read_i16be()?;
		let flags        = RangeFlags(file.read_i16be()?);
		let numTrue = file.read_u8()?;
		let numRegs = file.read_u8()?;

		let hasFade = flags.contains(RangeFlags::FADE);

		// Ensure chunk is actually large enough
		let required: usize = Self::SIZE as usize
			+ numTrue as usize * 4
			+ numRegs as usize * 2
			+ if hasFade { 2 } else { 0 };
		if required > size
		{
			return Err(io::Error::new(io::ErrorKind::Other, "Malformed chunk is too short"))
		}

		let mut bytesRead = Self::SIZE as usize;
		let mut trueRange = vec![EnhancedColour::default(); numTrue as usize];
		for i in &mut trueRange
		{
			i.cell = file.read_u8()?;
			bytesRead += 1 + file.read(&mut i.rgb)?;
		}

		let mut regsRange = vec![EnhancedIndex::default(); numRegs as usize];
		for i in &mut regsRange
		{
			i.cell  = file.read_u8()?;
			i.index = file.read_u8()?;
			bytesRead += 2;
		}

		// Read extended fade data if present
		let mut fadeExt = None;
		if hasFade
		{
			let numFade = file.read_u8()?;
			let fadePad = file.read_u8()?;
			// Ensure chunk is large enough for additional fade structures
			if required + numFade as usize * 2 > size
			{
				return Err(io::Error::new(io::ErrorKind::Other, "Malformed chunk is too short"))
			}

			bytesRead += 2;
			let mut fadeRange = vec![EnhancedFade::default(); numFade as usize];
			for i in &mut fadeRange
			{
				i.cell = file.read_u8()?;
				i.fade = file.read_u8()?;
				bytesRead += 2;
			}
			fadeExt = Some(EnhancedColourCycleFadeExt { numFade, fadePad, fadeRange })
		}

		Ok((ChunkReaders::EnhancedColourCycle(Self { min, max, rate, flags, numTrue, numRegs, trueRange, regsRange, fadeExt }), bytesRead))
	}

	const ID: [u8; 4] = *b"DRNG";
	const SIZE: u32 = 8;
}


optionset!
{
	pub(crate) struct RangeFlags: i16
	{
		NONE        = 0;
		ACTIVE      = 0b0001;
		REVERSE     = 0b0010;

		// DPaintIV/V DRNG flags
		DP_RESERVED = 0b0100;
		FADE        = 0b1000;
	}
}

optionset_custom_display!
{
	impl Display for RangeFlags
	{
		NONE =>        "None",
		ACTIVE =>      "Active",
		REVERSE =>     "Reverse",
		DP_RESERVED => "RNG_DP_RESERVED",
		FADE =>        "Fade"
	}
}
