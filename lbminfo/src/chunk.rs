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
use crate::lbm::LBMError;

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


pub(crate) enum ChunkReaders
{
	AnnotationText(strings::AnnotationText),
	NameText(strings::NameText),
	AuthorText(strings::AuthorText),
	CopyrightText(strings::CopyrightText),
	GenericText(strings::GenericText),
	LBMHeader(header::LBMHeader),
	ColourMap(standard::ColourMap),
	Grab(standard::Grab),
	CommodoreAmiga(amiga::CommodoreAmiga),
	CycleRange(range::CycleRange),
	CycleInfo(graphicraft::CycleInfo),
	EnhancedColourCycle(range::EnhancedColourCycle),
	DeluxePaintPrivateState(dpaint::DeluxePaintPrivateState),
	DeluxePaintPrivateExtended(dpaint::DeluxePaintPrivateExtended),
	DeluxePaintPerspective(dpaint::DeluxePaintPerspective),
	DeluxePaintThumbnail(dpaint::DeluxePaintThumbnail),
	DotsPerInch(nonstandard::DotsPerInch),
	UpdateSpans(custom::UpdateSpans),
	Body(standard::Body),
	PhotonPrivate(private::PhotonPrivate),
	PhotonPrivateScreens(private::PhotonPrivateScreens),
}


pub(crate) trait IFFChunk
{
	fn read(file: &mut fs::File, size: usize) -> io::Result<(ChunkReaders, usize)>;

	const ID: [u8; 4];
	const SIZE: u32;

	fn tryReadChunk(id: &[u8; 4], size: usize, file: &mut fs::File)
		-> Result<Option<(ChunkReaders, usize)>, LBMError>
	{
		if !id.eq(&Self::ID) { return Ok(None) }
		if size < Self::SIZE as usize { return Err(LBMError::BadChunk) }
		Ok(Some(Self::read(file, size)?))
	}
}


macro_rules! private_chunk
{
	($t:ident, $fourcc:expr) =>
	{
		pub(crate) struct $t;

		impl crate::chunk::IFFChunk for $t
		{
			fn read(_file: &mut std::fs::File, _size: usize) -> std::io::Result<(super::ChunkReaders, usize)>
			{
				Ok((super::ChunkReaders::$t($t {}), Self::SIZE as usize))
			}

			const ID: [u8; 4] = *$fourcc;
			const SIZE: u32 = 0;
		}
	}
}
pub(crate) use private_chunk;
