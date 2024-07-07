/* check.rs
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::fmt;
use crate::chunk::header::Compression;
use crate::chunk::amiga::AmigaViewportFlags;
use crate::chunk::range::RangeFlags;
use crate::lbm::{LBM, LBMType};


pub(crate) trait LBMCheck
{
	//TODO: file validation
	fn guess(&self) -> PlatformGuess;
}

impl LBMCheck for LBM
{
	fn guess(&self) -> PlatformGuess
	{
		// PBM originated in later versions of Deluxe Paint II for the PC
		if self.iffType == LBMType::CHUNKY
		{
			// Guess DPIIE if certain chunks are present
			if self.dpps.is_some() || self.dppv.is_some() || self.tiny.is_some()
			{
				return PlatformGuess::DOSDeluxePaint2Enhanced;
			}
			return PlatformGuess::DOSGeneric;
		}

		// Vertical RLE (VDAT) compression is used by Deluxe Paint for the Atari ST
		if self.header.compression == Compression::VERTICAL_RLE
		{
			return PlatformGuess::AtariST;
		}

		// The CAMG chunk means this is probably an Amiga ILBM
		if let Some(amiga) = &self.amiga
		{
			// Commodore Graphicraft loves to set SPRITES and uses a unique "Cycle Info" chunk
			if amiga.viewport & AmigaViewportFlags::SPRITES == AmigaViewportFlags::SPRITES
				&& self.cycleinfo.is_empty()
			{
				return PlatformGuess::AmigaGraphicraft;
			}

			// The "enhanced" colour cycle (DRNG) chunk originated in Deluxe Paint IV
			if !self.enhanced.is_empty()
			{
				// The undocumented "fade" bit was introduced in V, otherwise guess IV
				for i in &self.enhanced
				{
					if i.flags.contains(RangeFlags::FADE)
					{
						return PlatformGuess::AmigaDeluxePaintV;
					}
				}
				return PlatformGuess::AmigaDeluxePaintIV;
			}

			// Assume some version of Deluxe Paint from the undocumented "DPPS" chunk
			if self.dpps.is_some()
			{
				return PlatformGuess::AmigaDeluxePaint;
			}
			return PlatformGuess::AmigaGeneric;
		}

		// Standard Deluxe Paint II on MS-DOS used ILBM
		if self.dpps.is_some() || self.dppv.is_some() || self.tiny.is_some()
		{
			return PlatformGuess::DOSDeluxePaint2;
		}

		// If there are no Amiga-esque chunks and we have a very PC-ish resolution, guess PC
		//TODO: could perhaps be smarter, check aspect ratio?
		if self.grab.is_none() && self.enhanced.is_empty() && self.header.size == (320, 200)
		{
			return PlatformGuess::DOSGeneric;
		}

		return PlatformGuess::Unknown;
	}
}


pub(crate) enum PlatformGuess
{
	Unknown,

	AmigaGeneric,
	AmigaGraphicraft,
	AmigaDeluxePaint,
	AmigaDeluxePaintIV,
	AmigaDeluxePaintV,

	AtariST,

	DOSGeneric,
	DOSDeluxePaint2,
	DOSDeluxePaint2Enhanced,
}

impl fmt::Display for PlatformGuess
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", match self
		{
			Self::Unknown                 => "?",
			Self::AmigaGeneric            => "Amiga",
			Self::AmigaGraphicraft        => "Commodore Graphicraft",
			Self::AmigaDeluxePaint        => "Deluxe Paint (Amiga)",
			Self::AmigaDeluxePaintIV      => "Deluxe Paint IV (Amiga)",
			Self::AmigaDeluxePaintV       => "Deluxe Paint V (Amiga)",
			Self::AtariST                 => "Atari",
			Self::DOSGeneric              => "MS-DOS",
			Self::DOSDeluxePaint2         => "Deluxe Paint II (MS-DOS)",
			Self::DOSDeluxePaint2Enhanced => "Deluxe Paint II Enhanced"
		})
	}
}
