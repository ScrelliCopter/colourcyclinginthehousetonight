/* info.rs - Displays for ILBM information
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use std::fmt;
use crate::check::LBMCheck;
use crate::chunk::header::{Compression, Mask};
use crate::lbm::LBM;


pub(crate) trait LBMInfo
{
	fn shortInfo(&self) -> LBMShortInfo<'_>;
	fn fullInfo(&self) -> LBMFullInfo<'_>;
}

impl LBMInfo for LBM
{
	fn shortInfo(&self) -> LBMShortInfo<'_> { LBMShortInfo(self) }
	fn fullInfo(&self) -> LBMFullInfo<'_>   { LBMFullInfo(self) }
}


pub(crate) struct LBMShortInfo<'a>(&'a LBM);

impl fmt::Display for LBMShortInfo<'_>
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		let lbm = &self.0;

		let mut have = vec![];
		if lbm.annotation.is_some()  { have.push("ANNO"); }
		if lbm.name.is_some()        { have.push("NAME"); }
		if lbm.author.is_some()      { have.push("AUTH"); }
		if lbm.copyright.is_some()   { have.push("(c)"); }
		if lbm.palette.is_some()     { have.push("CMAP"); }
		if lbm.grab.is_some()        { have.push("GRAB"); }
		if lbm.amiga.is_some()       { have.push("CAMG"); }
		if !lbm.ranges.is_empty()    { have.push("CRNG"); }
		if !lbm.cycleinfo.is_empty() { have.push("CCRT"); }
		if !lbm.enhanced.is_empty()  { have.push("DRNG"); }
		if lbm.text.is_some()        { have.push("TEXT"); }
		if lbm.dpps.is_some()        { have.push("DPPS"); }
		if lbm.dpxt.is_some()        { have.push("DPXT"); }
		if lbm.dppv.is_some()        { have.push("DPPV"); }
		if lbm.tiny.is_some()        { have.push("TINY"); }
		if lbm.dpi.is_some()         { have.push("DPI"); }
		if lbm.spans.is_some()       { have.push("SPAN"); }
		if lbm.body.is_some()        { have.push("BODY"); }
		write!(fmt, "Type: \"{}\" ({}), Chunks: [{}]", lbm.iffType, lbm.guess(), have.join(","))?;
		if !lbm.unknown.is_empty()
		{
			write!(fmt, ", Unknown: [")?;
			for (idx, chunk) in lbm.unknown.iter().enumerate()
			{
				if idx > 0 { write!(fmt, ",")? }
				write!(fmt, "{}", String::from_utf8_lossy(chunk))?;
			}
			write!(fmt, "]")?;
		}
		return Ok(())
	}
}


pub(crate) struct LBMFullInfo<'a>(&'a LBM);

impl fmt::Display for LBMFullInfo<'_>
{
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		let lbm = &self.0;

		if let Some(anno) = &lbm.annotation
		{
			writeln!(f, "Annotation: \"{anno}\"")?;
		}
		if let Some(name) = &lbm.name
		{
			writeln!(f, "Name: \"{name}\"")?;
		}
		if let Some(author) = &lbm.author
		{
			writeln!(f, "Author: \"{author}\"")?;
		}
		if let Some(copyright) = &lbm.copyright
		{
			writeln!(f, "Copyright: \"{copyright}\"")?;
		}
		writeln!(f, "Type: {}", lbm.iffType)?;
		writeln!(f, "Header (BMHD):")?;
		writeln!(f, "  size:        {}x{}", lbm.header.size.0, lbm.header.size.1)?;
		if lbm.header.offset != (0, 0)
		{
			writeln!(f, "  origin:      x:{}, y:{}", lbm.header.offset.0, lbm.header.offset.1)?
		}
		writeln!(f, "  planes:      {}", lbm.header.numPlanes)?;
		if lbm.header.masking != Mask::NONE
		{
			writeln!(f, "  masking:     {}", lbm.header.masking)?
		}
		if lbm.header.compression != Compression::NONE
		{
			writeln!(f, "  compression: {}", lbm.header.compression)?
		}
		if lbm.header.pad1 != 0
		{
			writeln!(f, "  pad1:        {}", lbm.header.pad1)?
		}
		writeln!(f, "  transparent: {}", lbm.header.transparent)?;
		writeln!(f, "  aspectratio: {}:{}", lbm.header.aspect.0, lbm.header.aspect.1)?;
		writeln!(f, "  page size:   {}x{}", lbm.header.pageSize.0, lbm.header.pageSize.1)?;
		if let Some(palette) = &lbm.palette
		{
			writeln!(f, "Palette (CMAP):")?;
			let numEntries = palette.entries.len();
			writeln!(f, "  entries: {numEntries}")?;
			for (idx, triplet) in palette.entries.iter().enumerate()
			{
				if idx & 0x7 == 0
				{
					if idx > 0 { writeln!(f, )?; }
					write!(f, "  [{idx:>3}]:")?;
				}
				write!(f, " #{:02X}{:02X}{:02X}", triplet[0], triplet[1], triplet[2])?;
				if idx != numEntries - 1 { write!(f, ",")?; }
			}
			writeln!(f)?;
		}
		if let Some(grab) = &lbm.grab
		{
			writeln!(f, "Handle (GRAB):")?;
			writeln!(f, "  point: x: {}, y: {}", grab.point.0, grab.point.1)?;
		}
		if let Some(amiga) = &lbm.amiga
		{
			writeln!(f, "Amiga flags (CAMG):")?;
			if u32::from(amiga.viewport) & 0xFFFF0000 != 0
			{
				writeln!(f, "  Upper: 0x{:04X} (Reserved)", u32::from(amiga.viewport) >> 16)?;
			}
			writeln!(f, "  Lower: 0x{:04X} ({})", u32::from(amiga.viewport) & 0xFFFF, amiga.viewport)?;
		}
		if !lbm.ranges.is_empty() || !lbm.cycleinfo.is_empty() || !lbm.enhanced.is_empty()
		{
			writeln!(f, "Ranges:")?;
			for (idx, range) in lbm.ranges.iter().enumerate()
			{
				writeln!(f, "  [CRNG:{idx:>2}]")?;
				writeln!(f, "    pad1:  {}", range.pad1)?;
				writeln!(f, "    rate:  {}", range.rate)?;
				writeln!(f, "    flags: {}", range.flags)?;
				writeln!(f, "    low:   {}", range.low)?;
				writeln!(f, "    high:  {}", range.high)?;
			}
			for (idx, info) in lbm.cycleinfo.iter().enumerate()
			{
				writeln!(f, "  [CCRT:{idx:>2}]")?;
				writeln!(f, "    direction:    {}", info.direction)?;
				writeln!(f, "    start:        {}", info.start)?;
				writeln!(f, "    end:          {}", info.end)?;
				writeln!(f, "    seconds:      {}", info.time.seconds)?;
				writeln!(f, "    microseconds: {}", info.time.microseconds)?;
				writeln!(f, "    pad:          {}", info.pad)?;
			}
			for (idx, range) in lbm.enhanced.iter().enumerate()
			{
				writeln!(f, "  [DRNG:{idx:>2}]")?;
				writeln!(f, "    min:   {}", range.min)?;
				writeln!(f, "    max:   {}", range.max)?;
				writeln!(f, "    rate:  {}", range.rate)?;
				writeln!(f, "    flags: {}", range.flags)?;
				for (idx2, trueRange) in range.trueRange.iter().enumerate()
				{
					writeln!(f, "    [TRUE:{idx2:>2}] cell:{:>3}, colour: #{:02X}{:02X}{:02X}",
						trueRange.cell, trueRange.rgb[0], trueRange.rgb[1], trueRange.rgb[2])?;
				}
				for (idx2, regRange) in range.regsRange.iter().enumerate()
				{
					writeln!(f, "    [REG:{idx2:>2}] cell:{:>3}, index:{:>3}",
						regRange.cell, regRange.index)?;
				}
				if let Some(fadeExt) = &range.fadeExt
				{
					for (idx2, fadeRange) in fadeExt.fadeRange.iter().enumerate()
					{
						writeln!(f, "    [FADE:{idx2:>2}] cell:{:>3}, fade:{:>3}",
							fadeRange.cell, fadeRange.fade)?;
					}
				}
			}
		}
		if let Some(text) = &lbm.text
		{
			writeln!(f, "Generic (TEXT):")?;
			writeln!(f, "  \"{text}\"")?;
		}
		if let Some(dppv) = &lbm.dppv
		{
			writeln!(f, "DeluxePaint DOS perspective settings (DPPV):")?;
			writeln!(f, "  rotation type:       {}", dppv.rotType)?;
			writeln!(f, "  rotation angles:     {}, {}, {}", dppv.angle.x, dppv.angle.y, dppv.angle.z)?;
			writeln!(f, "  perspective depth:   {}", dppv.perspDepth)?;
			writeln!(f, "  perspective centre:  u:{}, v:{}", dppv.uvCentre.x, dppv.uvCentre.y)?;
			writeln!(f, "  fixed coordinate:    {}", dppv.fixedCoord)?;
			writeln!(f, "  large angle step:    {}", dppv.angleStep)?;
			writeln!(f, "  grid spacing:        x:{}, y:{}, z:{}", dppv.grid.x, dppv.grid.y, dppv.grid.z)?;
			writeln!(f, "  grid reset position: x:{}, y:{}, z:{}",
				dppv.gridReset.x, dppv.gridReset.y, dppv.gridReset.z)?;
			writeln!(f, "  grid brush centre:   x:{}, y:{}, z:{}",
				dppv.gridBrushCentre.x, dppv.gridBrushCentre.y, dppv.gridBrushCentre.z)?;
			writeln!(f, "  last brush centre:   x:{}, y:{}, z:{}",
				dppv.permBrushCentre.x, dppv.permBrushCentre.y, dppv.permBrushCentre.z)?;
			writeln!(f, "  rotation matrix:\n    {} {} {}\n    {} {} {}\n    {} {} {}",
				dppv.matrix.0, dppv.matrix.1, dppv.matrix.2,
				dppv.matrix.3, dppv.matrix.4, dppv.matrix.5,
				dppv.matrix.6, dppv.matrix.7, dppv.matrix.8)?;
		}
		if let Some(tiny) = &lbm.tiny
		{
			writeln!(f, "DeluxePaint DOS thumbnail (TINY):")?;
			writeln!(f, "  size:   {}x{}", tiny.size.0, tiny.size.1)?;
			writeln!(f, "  length: {} byte(s)", tiny.len)?;
		}
		if let Some(dpi) = &lbm.dpi
		{
			writeln!(f, "Dots per inch (DPI ):")?;
			writeln!(f, "  size: x: {}dpi, y: {}dpi", dpi.0.0, dpi.0.1)?;
		}
		if let Some(spans) = &lbm.spans
		{
			writeln!(f, "Update spans (SPAN):")?;
			for (idx, span) in spans.spans.iter().enumerate()
			{
				if span.left < 0 { continue }
				let idx = spans.startOffset as usize + idx;
				if span.innerLeft < 0
				{
					writeln!(f, "  [{}] {}-{}", idx, span.left, span.right)?;
				}
				else
				{
					writeln!(f, "  [{}] {}-{} {}-{}", idx,
						span.left, span.innerLeft,
						span.innerRight, span.right)?;
				}
			}
		}
		if let Some(body) = &lbm.body
		{
			if lbm.header.compression != Compression::NONE { write!(f, "Compressed ")?; }
			writeln!(f, "Raster data (BODY):")?;
			writeln!(f, "  length: {} byte(s)", body.len)?;
		}
		if !lbm.unknown.is_empty()
		{
			write!(f, "Unknown Chunks: [")?;
			for (idx, chunk) in lbm.unknown.iter().enumerate()
			{
				write!(f, "{}\"{}\"", if idx > 0 { ", " } else { "" }, String::from_utf8_lossy(chunk))?;
			}
			writeln!(f, "]")?;
		}
		Ok(())
	}
}
