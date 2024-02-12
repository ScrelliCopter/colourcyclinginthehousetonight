/* ibm.rs - Reader for "ILBM" type IFF FORMs
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::{fmt, fs, io, path};
use std::io::Seek;
use crate::chunk::header::LBMHeader;
use crate::chunk::standard::{Body, ColourMap, Grab};
use crate::chunk::amiga::CommodoreAmiga;
use crate::chunk::custom::UpdateSpans;
use crate::chunk::range::{CycleRange, EnhancedColourCycle};
use crate::chunk::graphicraft::CycleInfo;
use crate::chunk::dpaint::{DeluxePaintPerspective, DeluxePaintPrivateExtended, DeluxePaintPrivateState, DeluxePaintThumbnail};
use crate::chunk::strings::{AnnotationText, AuthorText, CopyrightText, GenericText, NameText};
use crate::chunk::nonstandard::DotsPerInch;
use crate::chunk::IFFChunk;
use crate::fsext::FileExt;


// https://web.archive.org/web/20070911223155/http://www.szonye.com/bradd/iff.html
pub(crate) struct LBM
{
	pub(crate) iffType: LBMType,
	pub(crate) annotation: Option<AnnotationText>,
	pub(crate) name: Option<NameText>,
	pub(crate) author: Option<AuthorText>,
	pub(crate) copyright: Option<CopyrightText>,
	pub(crate) text: Option<GenericText>,
	pub(crate) header: LBMHeader,
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
	pub(crate) body: Option<Body>
}

fn tryReadChunk<T: IFFChunk>(id: &[u8; 4], size: usize, file: &mut fs::File)
	-> Result<Option<(T, usize)>, LBMError>
{
	if !id.eq(&T::ID) { return Ok(None) }
	if size < T::SIZE as usize { return Err(LBMError::BadChunk) }
	Ok(Some(T::read(file, size)?))
}

macro_rules! tryPut
{
	($out:ident, $in:expr) =>
	{
		if $out.is_some()
		{
			return Err(LBMError::ChunkConflict)
		}
		$out = Option::from($in);
	};
}

impl LBM
{
	pub(crate) fn read(path: &path::Path) -> Result<LBM, LBMError>
	{
		let mut file = fs::File::open(&path)?;  // Open the file

		// Valid IFFs should open with "FORM"
		//TODO: Basic support for "CAT " and "LIST"
		if &file.read_fourcc()? != b"FORM" { return Err(LBMError::BadForm) }
		let formSize = file.read_u32be()? as usize;
		// Form shouldn't be smaller than smallest possible image
		if formSize < (4 + LBMHeader::SIZE as usize + 8 * 2) { return Err(LBMError::BadForm) }

		let iffType = LBMType(file.read_fourcc()?);
		// Check form type
		if ![LBMType::PLANAR, LBMType::CHUNKY].contains(&iffType)
		{
			return Err(LBMError::BadType)
		}

		let mut annotation = None;
		let mut name = None;
		let mut author = None;
		let mut copyright = None;
		let mut header = None;
		let mut palette = None;
		let mut grab = None;
		let mut amiga = None;
		let mut ranges = vec!();
		let mut cycleinfo = vec!();
		let mut enhanced = vec!();
		let mut text = None;
		let mut dppv = None;
		let mut dpps = None;
		let mut dpxt = None;
		let mut tiny = None;
		let mut dpi = None;
		let mut spans = None;
		let mut body = None;

		let mut bytesRead: usize = 4;
		while bytesRead < formSize
		{
			let chunkId = file.read_fourcc()?;
			let chunkSize = file.read_u32be()? as usize;
			let realSize: usize = chunkSize  + (chunkSize & 0x1);  // Round up to word boundaries

			// Read all chunks in the IFF FORM
			let chunkRead: usize;
			if let Some((chunk, size)) = tryReadChunk::<AnnotationText>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(annotation, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<NameText>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(name, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<AuthorText>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(author, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<CopyrightText>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(copyright, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<GenericText>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(text, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<LBMHeader>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(header, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<ColourMap>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(palette, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<Grab>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(grab, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<CommodoreAmiga>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(amiga, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<CycleRange>(&chunkId, chunkSize, &mut file)?
			{
				ranges.push(chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<CycleInfo>(&chunkId, chunkSize, &mut file)?
			{
				cycleinfo.push(chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<EnhancedColourCycle>(&chunkId, chunkSize, &mut file)?
			{
				enhanced.push(chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<DeluxePaintPrivateState>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(dpps, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<DeluxePaintPrivateExtended>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(dpxt, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<DeluxePaintPerspective>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(dppv, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<DeluxePaintThumbnail>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(tiny, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<DotsPerInch>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(dpi, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<UpdateSpans>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(spans, chunk);
				chunkRead = size;
			}
			else if let Some((chunk, size)) = tryReadChunk::<Body>(&chunkId, chunkSize, &mut file)?
			{
				tryPut!(body, chunk);
				chunkRead = size;
			}
			else
			{
				// Skip unrecognised chunks
				if ![b"JUNK", b"SNFO", b"OGGV", b"SNFO"].contains(&&chunkId)
				{
					eprintln!("Unknown chunk: \"{}\"", String::from_utf8_lossy(&chunkId));
				}
				chunkRead = 0;
			}

			if realSize > chunkRead
			{
				file.seek(io::SeekFrom::Current((realSize - chunkRead) as i64))?;
			}
			bytesRead += 8 + realSize;
		}

		Ok(LBM {
			iffType,
			annotation, name, author, copyright,
			header: header.ok_or(LBMError::MissingHeader)?,
			palette,
			grab,
			amiga,
			ranges, cycleinfo, enhanced,
			text,
			dpps, dpxt, dppv, tiny,
			spans,
			dpi,
			body })
	}
}


pub(crate) struct LBMType([u8; 4]);

// Note:
impl LBMType
{
	pub(crate) const PLANAR: LBMType            = LBMType(*b"ILBM");
	pub(crate) const CHUNKY: LBMType            = LBMType(*b"PBM ");
	//pub(crate) const PLANAR_CONTIGUOUS: LBMType = LBMType(*b"ACBM");
	//pub(crate) const CHUNKY_TVPAINT: LBMType    = LBMType(*b"DEEP");
}

impl PartialEq for LBMType { fn eq(&self, rhs: &Self) -> bool { self.0 == rhs.0 } }

impl fmt::Display for LBMType
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "{}", String::from_utf8_lossy(&self.0))
	}
}


#[derive(Debug)]
pub(crate) enum LBMError
{
	FileError(io::Error),
	BadForm,
	BadType,
	BadChunk,
	MissingHeader,
	ChunkConflict
}

impl From<io::Error> for LBMError
{
	fn from(err: io::Error) -> Self { Self::FileError(err) }
}
