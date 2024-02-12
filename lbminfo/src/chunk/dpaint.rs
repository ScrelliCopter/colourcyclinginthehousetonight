/* dpaint.rs - Various non-standard Deluxe Paint chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fmt, fs, io};
use crate::chunk::{IFFChunk, private_chunk};
use crate::fsext::FileExt;


// FIXME: Unknown private Deluxe Paint chunks
private_chunk!(DeluxePaintPrivateState, b"DPPS");
private_chunk!(DeluxePaintPrivateExtended, b"DPXT");


// https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#ILBM.DPPV
pub(crate) struct DeluxePaintPerspective
{
	pub(crate) rotType: RotationType,   // Rotation type
	pub(crate) angle: (i16, i16, i16),  // Rotation angles (in degrees)
	pub(crate) perspDepth: i32,         // Perspective depth
	pub(crate) uvCentre: (i16, i16),    // Coords of centre perspective,
	                                    //  relative to backing bitmap in virtual coords.
	pub(crate) fixedCoord: i16,         // Which coordinate is fixed
	pub(crate) angleStep: i16,          // Large angle stepping amount
	pub(crate) grid: Vec3,              // Grid spacing
	pub(crate) gridReset: Vec3,         // Where the grid goes on reset
	pub(crate) gridBrushCentre: Vec3,   // Brush centre when grid was last on, as reference point
	pub(crate) permBrushCentre: Vec3,   // Brush centre the last time the mouse button was clicked,
	                                    //  a rotation was performed, or motion along "fixed" axis.
	pub(crate) matrix: Mat3             // Rotation matrix (TODO: column-major or row-major??)
}

impl IFFChunk for DeluxePaintPerspective
{
	fn read(file: &mut fs::File, _size: usize) -> Result<(Self, usize), io::Error>
	{
		Ok((Self {
			rotType:         RotationType(file.read_i16be()?),
			angle:           (file.read_i16be()?, file.read_i16be()?, file.read_i16be()?),
			perspDepth:      file.read_i32be()?,
			uvCentre:        (file.read_i16be()?, file.read_i16be()?),
			fixedCoord:      file.read_i16be()?,
			angleStep:       file.read_i16be()?,
			grid:            (file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			gridReset:       (file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			gridBrushCentre: (file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			permBrushCentre: (file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			matrix:
				(file.read_i32be()?, file.read_i32be()?, file.read_i32be()?,
				 file.read_i32be()?, file.read_i32be()?, file.read_i32be()?,
				 file.read_i32be()?, file.read_i32be()?, file.read_i32be()?)
		}, Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"DPPV";
	const SIZE: u32 = 104;
}


#[derive(PartialEq, Eq)]
pub(crate) struct RotationType(i16);

impl RotationType
{
	const EULER: RotationType = RotationType(0);
	const INCR: RotationType  = RotationType(1);
}

impl Default for RotationType { fn default() -> Self { RotationType::EULER } }

impl fmt::Display for RotationType
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", match *self
		{
			Self::EULER => "ROT_EULER",
			Self::INCR  => "ROT_INCR",
			_ => { panic!("Shouldn't happen") }
		})
	}
}


type Vec3 = (i32, i32, i32);
type Mat3 = (
	i32, i32, i32,
	i32, i32, i32,
	i32, i32, i32);


// Deluxe Paint II (MS-DOS) embedded thumbnail chunk
// https://moddingwiki.shikadi.net/wiki/LBM_Format#TINY:_Thumbnail
pub(crate) struct DeluxePaintThumbnail
{
	pub(crate) size: (u16, u16),
	pub(crate) len: usize
}

impl IFFChunk for DeluxePaintThumbnail
{
	fn read(file: &mut fs::File, size: usize) -> Result<(Self, usize), io::Error>
	{
		let chunk = Self {
			size: (file.read_u16be()?, file.read_u16be()?),
			len: size - 4 };
		return Ok((chunk, Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"TINY";
	const SIZE: u32 = 4;
}
