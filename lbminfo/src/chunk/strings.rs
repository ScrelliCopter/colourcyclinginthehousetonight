/* strings.rs - Quasi-standard string chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fmt, fs, io};
use std::io::Read;
use crate::chunk::IFFChunk;


// "IFF EA 85 generic author/name/copyright chunk(s)"
// https://wiki.amigaos.net/wiki/IFF_FORM_and_Chunk_Registry
// https://www.fileformat.info/format/iff/egff.htm
macro_rules! implementTextChunk
{
	($t:ident, $id:expr) =>
	{
		pub(crate) struct $t(PlatformText);

		impl IFFChunk for $t
		{
			fn read(file: &mut fs::File, size: usize) -> io::Result<(Self, usize)>
			{
				Ok(($t(PlatformText::from(file, size)?), size))
			}

			const ID: [u8; 4] = *$id;
			const SIZE: u32 = 0;
		}

		impl fmt::Display for $t
		{
			fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { self.0.fmt(f) }
		}
	};
}

implementTextChunk!(AnnotationText, b"ANNO");
implementTextChunk!(NameText,       b"NAME");
implementTextChunk!(AuthorText,     b"AUTH");
implementTextChunk!(CopyrightText,  b"(c) ");
implementTextChunk!(GenericText,    b"TEXT");


// Platform defined 8-bit single-byte text,
//  probably ASCII and likely similar to ISO 8859-1 in most cases
// https://www.iana.org/assignments/charset-reg/Amiga-1251
// https://www.martinreddy.net/gfx/2d/IFF.txt
struct PlatformText(Vec<u8>);

impl fmt::Display for PlatformText
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", self.0.iter()
			.take_while(|c| **c != 0)
			.map(|&c| c as char)
			.collect::<String>())
	}
}

impl PlatformText
{
	fn from(file: &mut fs::File, size: usize) -> io::Result<PlatformText>
	{
		let mut payload = vec![0u8; size];
		file.read(&mut payload)?;
		Ok(PlatformText(payload))
	}
}
