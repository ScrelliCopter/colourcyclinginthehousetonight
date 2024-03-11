/* reader.rs - Reader for "ILBM" type IFF FORMs
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::fs;
use crate::lbm::LBMError;
use crate::chunk::{ChunkReaders, IFFChunk};
use crate::chunk::header::LBMHeader;
use crate::chunk::standard::{Body, ColourMap, Grab};
use crate::chunk::amiga::CommodoreAmiga;
use crate::chunk::custom::UpdateSpans;
use crate::chunk::range::{CycleRange, EnhancedColourCycle};
use crate::chunk::graphicraft::CycleInfo;
use crate::chunk::dpaint::{DeluxePaintPerspective, DeluxePaintPrivateExtended, DeluxePaintPrivateState, DeluxePaintThumbnail};
use crate::chunk::strings::{AnnotationText, AuthorText, CopyrightText, GenericText, NameText};
use crate::chunk::nonstandard::DotsPerInch;


#[derive(Default)]
pub(crate) struct LBMReader
{
	pub(crate) annotation: Option<AnnotationText>,
	pub(crate) name: Option<NameText>,
	pub(crate) author: Option<AuthorText>,
	pub(crate) copyright: Option<CopyrightText>,
	pub(crate) text: Option<GenericText>,
	pub(crate) header: Option<LBMHeader>,
	pub(crate) palette: Option<ColourMap>,
	pub(crate) grab: Option<Grab>,
	pub(crate) amiga: Option<CommodoreAmiga>,
	pub(crate) ranges: Vec<CycleRange>,
	pub(crate) cycleinfo: Vec<CycleInfo>,
	pub(crate) enhanced: Vec<EnhancedColourCycle>,
	pub(crate) dpps: Option<DeluxePaintPrivateState>,
	pub(crate) dpxt: Option<DeluxePaintPrivateExtended>,
	pub(crate) dppv: Option<DeluxePaintPerspective>,
	pub(crate) tiny: Option<DeluxePaintThumbnail>,
	pub(crate) dpi: Option<DotsPerInch>,
	pub(crate) spans: Option<UpdateSpans>,
	pub(crate) body: Option<Body>,
}

macro_rules! tryPut
{
	($out:expr, $in:expr) =>
	{
		if $out.is_some()
		{
			return Err(LBMError::ChunkConflict)
		}
		$out = Option::from($in);
	};
}

impl LBMReader
{
	pub(crate) fn readChunk(&mut self, chunkId: &[u8; 4], chunkSize: usize, file: &mut fs::File)
		-> Result<Option<usize>, LBMError>
	{
		let readers =
		[
			AnnotationText::tryReadChunk,
			NameText::tryReadChunk,
			AuthorText::tryReadChunk,
			CopyrightText::tryReadChunk,
			GenericText::tryReadChunk,
			LBMHeader::tryReadChunk,
			ColourMap::tryReadChunk,
			Grab::tryReadChunk,
			CommodoreAmiga::tryReadChunk,
			CycleRange::tryReadChunk,
			CycleInfo::tryReadChunk,
			EnhancedColourCycle::tryReadChunk,
			DeluxePaintPrivateState::tryReadChunk,
			DeluxePaintPrivateExtended::tryReadChunk,
			DeluxePaintPerspective::tryReadChunk,
			DeluxePaintThumbnail::tryReadChunk,
			DotsPerInch::tryReadChunk,
			UpdateSpans::tryReadChunk,
			Body::tryReadChunk,
		];

		for reader in readers
		{
			if let Some((reader, bytesRead)) = reader(chunkId, chunkSize, file)?
			{
				match reader
				{
					ChunkReaders::AnnotationText(chunk) => { tryPut!(self.annotation, chunk); },
					ChunkReaders::NameText(chunk) => { tryPut!(self.name, chunk); },
					ChunkReaders::AuthorText(chunk) => { tryPut!(self.author, chunk); },
					ChunkReaders::CopyrightText(chunk) => { tryPut!(self.copyright, chunk); },
					ChunkReaders::GenericText(chunk) => { tryPut!(self.text, chunk); },
					ChunkReaders::LBMHeader(chunk) => { tryPut!(self.header, chunk); },
					ChunkReaders::ColourMap(chunk) => { tryPut!(self.palette, chunk); },
					ChunkReaders::Grab(chunk) => { tryPut!(self.grab, chunk); },
					ChunkReaders::CommodoreAmiga(chunk) => { tryPut!(self.amiga, chunk); },
					ChunkReaders::CycleRange(chunk) => self.ranges.push(chunk),
					ChunkReaders::CycleInfo(chunk) => self.cycleinfo.push(chunk),
					ChunkReaders::EnhancedColourCycle(chunk) => self.enhanced.push(chunk),
					ChunkReaders::DeluxePaintPrivateState(chunk) => { tryPut!(self.dpps, chunk); },
					ChunkReaders::DeluxePaintPrivateExtended(chunk) => { tryPut!(self.dpxt, chunk); },
					ChunkReaders::DeluxePaintPerspective(chunk) => { tryPut!(self.dppv, chunk); },
					ChunkReaders::DeluxePaintThumbnail(chunk) => { tryPut!(self.tiny, chunk); },
					ChunkReaders::DotsPerInch(chunk) => { tryPut!(self.dpi, chunk); },
					ChunkReaders::UpdateSpans(chunk) => { tryPut!(self.spans, chunk); },
					ChunkReaders::Body(chunk) => { tryPut!(self.body, chunk); },
					_ => continue
				}
				return Ok(Some(bytesRead));
			}
		}
		Ok(None)
	}
}
