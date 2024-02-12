/* custom.rs - colourcyclinginthehousetonight custom (non-standard) chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fs, io};
use crate::chunk::IFFChunk;
use crate::fsext::FileExt;


pub(crate) struct UpdateSpans
{
	pub(crate) startOffset: i16,
	#[allow(dead_code)]
	pub(crate) numSpans: i16,
	pub(crate) spans: Vec<Span>
}

impl IFFChunk for UpdateSpans
{
	fn read(file: &mut fs::File, _size: usize) -> io::Result<(Self, usize)>
	{
		let startOffset = file.read_i16be()?;
		let numSpans = file.read_i16be()?;
		let mut spans = vec![Span::default(); numSpans as usize];
		let mut bytesRead = Self::SIZE as usize;
		for span in &mut spans
		{
			span.left       = file.read_i16be()?;
			if span.left < 0 { bytesRead += 2; continue; }
			span.right      = file.read_i16be()?;
			span.innerLeft  = file.read_i16be()?;
			if span.innerLeft < 0 { bytesRead += 6; continue; }
			span.innerRight = file.read_i16be()?;
			bytesRead += 8;
		}
		Ok((Self { startOffset, numSpans, spans }, bytesRead))
	}

	const ID: [u8; 4] = *b"SPAN";
	const SIZE: u32 = 4;
}


#[derive(Default, Clone)]
pub(crate) struct Span
{
	pub(crate) left: i16,
	pub(crate) right: i16,
	pub(crate) innerLeft: i16,
	pub(crate) innerRight: i16
}
