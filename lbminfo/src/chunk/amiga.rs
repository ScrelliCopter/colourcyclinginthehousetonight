/* amiga.rs - Standard ILBM Amiga "CAMG" chunk
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fs, io};
use super::{ChunkReaders, IFFChunk};
use crate::fsext::FileExt;
use crate::optionset::{optionset, optionset_custom_display};


// http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html
pub(crate) struct CommodoreAmiga { pub(crate) viewport: AmigaViewportFlags }

impl IFFChunk for CommodoreAmiga
{
	fn read(file: &mut fs::File, _size: usize) -> io::Result<(ChunkReaders, usize)>
	{
		Ok((ChunkReaders::CommodoreAmiga(Self { viewport: AmigaViewportFlags(file.read_u32be()?) }),
			Self::SIZE as usize))
	}

	const ID: [u8; 4] = *b"CAMG";
	const SIZE: u32 = 4;
}


optionset!
{
	pub(crate) struct AmigaViewportFlags: u32
	{
		GENLOCK_VIDEO   = 0b0000000000000010;
		LACE            = 0b0000000000000100;
		DOUBLESCAN      = 0b0000000000001000;
		SUPERHIRES      = 0b0000000000100000;
		PFBA            = 0b0000000001000000;
		EXTRA_HALFBRITE = 0b0000000010000000;
		GENLOCK_AUDIO   = 0b0000000100000000;
		DUALPF          = 0b0000010000000000;
		HAM             = 0b0000100000000000;
		EXTENDED_MODE   = 0b0001000000000000;
		VP_HIDE         = 0b0010000000000000;
		SPRITES         = 0b0100000000000000;
		HIRES           = 0b1000000000000000;
	}
}

//optionset_default_display!(AmigaViewportFlags);
optionset_custom_display!
{
	impl Display for AmigaViewportFlags
	{
		GENLOCK_VIDEO   => "GENLOCK_VIDEO",
		LACE            => "LACE",
		DOUBLESCAN      => "DOUBLESCAN",
		SUPERHIRES      => "SUPERHIRES",
		PFBA            => "PFBA",
		EXTRA_HALFBRITE => "EXTRA_HALFBRITE",
		GENLOCK_AUDIO   => "GENLOCK_AUDIO",
		DUALPF          => "DUALPF",
		HAM             => "HAM",
		EXTENDED_MODE   => "EXTENDED_MODE",
		VP_HIDE         => "VP_HIDE",
		SPRITES         => "SPRITES",
		HIRES           => "HIRES",
	}
}
