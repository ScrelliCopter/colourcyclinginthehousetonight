/* graphicraft.rs - Non-standard Commodore Graphicraft chunk(s)
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::fmt::{Display, Formatter};
use std::fs::File;
use std::io::Error;
use crate::chunk::IFFChunk;
use crate::fsext::FileExt;


// https://1fish2.github.io/IFF/IFF%20docs%20with%20Commodore%20revisions/ILBM.pdf
pub(crate) struct CycleInfo
{
	pub(crate) direction: Direction,
	pub(crate) start: u8,
	pub(crate) end: u8,
	pub(crate) time: CycleTime,
	pub(crate) pad: i16
}

impl IFFChunk for CycleInfo
{
	fn read(file: &mut File, _size: usize) -> Result<(Self, usize), Error>
	{
		Ok((Self {
			direction: Direction(file.read_i16be()?),
			start:     file.read_u8()?,
			end:       file.read_u8()?,
			time:      CycleTime {
				seconds:      file.read_i32be()?,
				microseconds: file.read_i32be()? },
			pad: file.read_i16be()?,
		}, Self::SIZE as usize))
	}
	const ID: [u8; 4] = *b"CCRT";
	const SIZE: u32 = 14;
}


#[derive(PartialEq, Eq)]
pub(crate) struct Direction(i16);

impl Direction
{
	const NONE: Direction     = Direction( 0);
	const FORWARD: Direction  = Direction( 1);
	const BACKWARD: Direction = Direction(-1);
}

impl Display for Direction
{
	fn fmt(&self, fmt: &mut Formatter<'_>) -> std::fmt::Result
	{
		write!(fmt, "{}", match *self
		{
			Self::NONE     => "None (disabled)",
			Self::FORWARD  => "Forward",
			Self::BACKWARD => "Backward",
			_ => { panic!("Shouldn't happen") }
		})
	}
}


pub(crate) struct CycleTime
{
	pub(crate) seconds: i32,
	pub(crate) microseconds: i32
}
