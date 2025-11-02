/* dpaint.rs - Various non-standard Deluxe Paint chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fmt, fs, io};
use super::{ChunkReaders, IFFChunk, private_chunk};
use crate::fsext::FileExt;
use crate::maths::{vec2::Vec2, vec3::{Vec3, Vec3i}, mat3::Mat3};


// FIXME: Unknown private Deluxe Paint chunks
private_chunk!(DeluxePaintPrivateState, b"DPPS");
private_chunk!(DeluxePaintPrivateExtended, b"DPXT");


// https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#ILBM.DPPV
pub(crate) struct DeluxePaintPerspective
{
	pub(crate) rotType: RotationType,   // Rotation type
	pub(crate) angle: Vec3<i16>,        // Rotation angles (in degrees)
	pub(crate) perspDepth: i32,         // Perspective depth
	pub(crate) uvCentre: Vec2<i16>,     // Coords of centre perspective,
	                                    //  relative to backing bitmap in virtual coords.
	pub(crate) fixedCoord: i16,         // Which coordinate is fixed
	pub(crate) angleStep: i16,          // Large angle stepping amount
	pub(crate) grid: Vec3i,             // Grid spacing
	pub(crate) gridReset: Vec3i,        // Where the grid goes on reset
	pub(crate) gridBrushCentre: Vec3i,  // Brush centre when grid was last on, as reference point
	pub(crate) permBrushCentre: Vec3i,  // Brush centre the last time the mouse button was clicked,
	                                    //  a rotation was performed, or motion along "fixed" axis.
	pub(crate) matrix: Mat3<i32>        // Rotation matrix (TODO: column-major or row-major??)
}

impl IFFChunk for DeluxePaintPerspective
{
	fn read(file: &mut fs::File, _size: usize) -> Result<(ChunkReaders, usize), io::Error>
	{
		Ok((ChunkReaders::DeluxePaintPerspective(Self {
			rotType:         RotationType(file.read_i16be()?),
			angle:           Vec3::new(file.read_i16be()?, file.read_i16be()?, file.read_i16be()?),
			perspDepth:      file.read_i32be()?,
			uvCentre:        Vec2::new(file.read_i16be()?, file.read_i16be()?),
			fixedCoord:      file.read_i16be()?,
			angleStep:       file.read_i16be()?,
			grid:            Vec3::new(file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			gridReset:       Vec3::new(file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			gridBrushCentre: Vec3::new(file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			permBrushCentre: Vec3::new(file.read_i32be()?, file.read_i32be()?, file.read_i32be()?),
			matrix: Mat3(
				file.read_i32be()?, file.read_i32be()?, file.read_i32be()?,
				file.read_i32be()?, file.read_i32be()?, file.read_i32be()?,
				file.read_i32be()?, file.read_i32be()?, file.read_i32be()?)
		}), Self::SIZE as usize))
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


// Deluxe Paint II (MS-DOS) embedded thumbnail chunk
// https://moddingwiki.shikadi.net/wiki/LBM_Format#TINY:_Thumbnail
pub(crate) struct DeluxePaintThumbnail
{
	pub(crate) size: (u16, u16),
	pub(crate) len: usize
}

impl IFFChunk for DeluxePaintThumbnail
{
	fn read(file: &mut fs::File, size: usize) -> Result<(ChunkReaders, usize), io::Error>
	{
		let chunk = Self {
			size: (file.read_u16be()?, file.read_u16be()?),
			len: size - 4 };
		return Ok((ChunkReaders::DeluxePaintThumbnail(chunk), Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"TINY";
	const SIZE: u32 = 4;
}
