/* chunk.rs - Base trait and helpers for IFF chunk structures
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

pub(crate) mod header;
pub(crate) mod standard;
pub(crate) mod strings;
pub(crate) mod amiga;
pub(crate) mod range;
pub(crate) mod graphicraft;
pub(crate) mod dpaint;
pub(crate) mod private;
pub(crate) mod custom;
pub(crate) mod nonstandard;

use std::{io, fs};

/* TODO:

 -- need test files for these --
"DEST"
"SPRT"

seen:

documented:
"ABIT" // https://wiki.multimedia.cx/index.php?title=IFF#ACBM_and_ABIT http://fileformats.archiveteam.org/wiki/ILBM#ACBM
"CLUT" // Color Lookup Table chunk
"CTBL" // Newtek Dynamic HAM color chunks - Uses CTBL and DYCP chunks.
"DYCP" // Newtek Dynamic HAM color chunks
"PCHG" // (Palette Changes) Line by line palette control information (Sebastiano Vigna)
"SHAM" // (Sliced HAM) - Uses a SHAM chunk.

private:
"ASDG" // private ASDG application chunk (Art Department Professional (ADPro)?)
"BHCP" // private Photon Paint chunk (screens)
"BHSM" // private Photon Paint chunk

unknown:
"ALFA" // Alfa Data AlfaScan?
"ALHD"
"ALPH"
"BEAM"
"BRNG"
"DPII"
"IMRT"
"LOCK"
"MPCT"
"OPCF"
"OPCP"
"OVTN"
"PSIM"
"XS24"
*/


pub(crate) trait IFFChunk
{
	fn read(file: &mut fs::File, size: usize) -> io::Result<(Self, usize)> where Self: Sized;

	const ID: [u8; 4];
	const SIZE: u32;
}


macro_rules! private_chunk
{
	($t:ident, $fourcc:expr) =>
	{
		pub(crate) struct $t;

		impl crate::chunk::IFFChunk for $t
		{
			fn read(_file: &mut std::fs::File, _size: usize) -> std::io::Result<(Self, usize)>
			{
				Ok(($t {}, Self::SIZE as usize))
			}

			const ID: [u8; 4] = *$fourcc;
			const SIZE: u32 = 0;
		}
	}
}
pub(crate) use private_chunk;
