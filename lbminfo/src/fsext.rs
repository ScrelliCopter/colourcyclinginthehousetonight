/* fsext.rs - Extensions to fs::File for reading Big-endian IFF files
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::fs::File;
use std::io::{Error, ErrorKind, Read};
use std::mem::size_of;


macro_rules! fixed_read
{
	($f:ident, $t:ty, $n:expr) =>
	{
		{
			let mut tmp: [$t; $n] = [0; $n];
			if $f.read(&mut tmp)? != $n
			{
				return Err(Error::new(ErrorKind::UnexpectedEof, "Underread"))
			}
			tmp
		}
	};
}

macro_rules! implement_fixed_size_read
{
	($name:ident, $t:ty) =>
	{
		fn $name(&mut self) -> Result<$t, Error>
		{
			Ok(<$t>::from_ne_bytes(fixed_read!(self, u8, size_of::<$t>())))
		}
	};
	($name:ident, $t:ty, $n:expr) =>
	{
		fn $name(&mut self) -> Result<[$t; $n], Error>
		{
			Ok(fixed_read!(self, $t, $n).clone())
		}
	};
}

macro_rules! implement_fixed_size_read_be
{
	($name:ident, $t:ty) =>
	{
		fn $name(&mut self) -> Result<$t, Error>
		{
			Ok(<$t>::from_be_bytes(fixed_read!(self, u8, size_of::<$t>())))
		}
	};
}

pub(crate) trait FileExt
{
	fn read_fourcc(&mut self) -> Result<[u8; 4], Error>;
	fn read_u32be(&mut self) -> Result<u32, Error>;
	fn read_u16be(&mut self) -> Result<u16, Error>;
	fn read_u8(&mut self) -> Result<u8, Error>;
	fn read_i32be(&mut self) -> Result<i32, Error>;
	fn read_i16be(&mut self) -> Result<i16, Error>;
	fn read_i8(&mut self) -> Result<i8, Error>;
}

impl FileExt for File
{
	implement_fixed_size_read!(read_fourcc, u8, 4);
	implement_fixed_size_read_be!(read_u32be, u32);
	implement_fixed_size_read_be!(read_i32be, i32);
	implement_fixed_size_read_be!(read_u16be, u16);
	implement_fixed_size_read_be!(read_i16be, i16);
	implement_fixed_size_read!(read_u8, u8);
	implement_fixed_size_read!(read_i8, i8);
}
