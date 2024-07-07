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
use crate::reader::LBMReader;


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

		let mut lbm = LBMReader::default();
		let mut bytesRead: usize = 4;
		while bytesRead < formSize
		{
			let chunkId = file.read_fourcc()?;
			let chunkSize = file.read_u32be()? as usize;
			let realSize: usize = chunkSize  + (chunkSize & 0x1);  // Round up to word boundaries

			// Read all chunks in the IFF FORM
			let chunkRead = lbm.readChunk(&chunkId, chunkSize, &mut file)?.unwrap_or_else(||
			{
				// Skip unrecognised chunks
				if ![b"JUNK", b"SNFO", b"OGGV", b"SNFO"].contains(&&chunkId)
				{
					eprintln!("Unknown chunk: \"{}\"", String::from_utf8_lossy(&chunkId));
				}
				return 0;
			});
			if realSize > chunkRead
			{
				file.seek(io::SeekFrom::Current((realSize - chunkRead) as i64))?;
			}
			bytesRead += 8 + realSize;
		}

		Ok(LBM {
			iffType,
			annotation: lbm.annotation, name: lbm.name, author: lbm.author, copyright: lbm.copyright,
			header: lbm.header.ok_or(LBMError::MissingHeader)?,
			palette: lbm.palette,
			grab: lbm.grab,
			amiga: lbm.amiga,
			ranges: lbm.ranges, cycleinfo: lbm.cycleinfo, enhanced: lbm.enhanced,
			text: lbm.text,
			dpps: lbm.dpps, dpxt: lbm.dpxt, dppv: lbm.dppv, tiny: lbm.tiny,
			spans: lbm.spans,
			dpi: lbm.dpi,
			body: lbm.body })
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
	#[allow(dead_code)]
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
