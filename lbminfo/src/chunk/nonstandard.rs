/* private.rs - Various nonstandard chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fs, io};
use crate::chunk::IFFChunk;
use crate::fsext::FileExt;


// https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#ILBM.DPI
pub(crate) struct DotsPerInch(pub(crate) (u16, u16));

impl IFFChunk for DotsPerInch
{
	fn read(file: &mut fs::File, _size: usize) -> io::Result<(Self, usize)>
	{
		Ok((Self((file.read_u16be()?, file.read_u16be()?)), Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"DPI ";
	const SIZE: u32 = 4;
}
