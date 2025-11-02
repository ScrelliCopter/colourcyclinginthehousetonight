/* main.rs
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

#![allow(non_snake_case)]

mod chunk;
mod lbm;
mod reader;
mod fsext;
mod info;
mod check;
mod optionset;

mod maths
{
	pub(crate) mod vec2;
	pub(crate) mod vec3;
	pub(crate) mod mat3;
}

use crate::info::LBMInfo;
use crate::lbm::LBM;

use jaarg::{Opt, Opts, ParseControl, ParseResult};
use std::io::Error;
use std::path::{Path, PathBuf};
use std::process::ExitCode;
use std::fs;

enum ParseArgsResult
{
	Continue(bool, PathBuf),
	Exit(ExitCode),
}

fn parseArgs() -> ParseArgsResult
{
	let mut path = PathBuf::new();
	let mut scan = false;

	enum Arg { Help, Path, Scan }
	const OPTIONS: Opts<Arg> = Opts::new(
	&[
		Opt::help_flag(Arg::Help, &["-h", "--help"]),
		Opt::positional(Arg::Path, "path").required()
			.help_text("Path to an ILBM/PBM, or a directory path (when scanning)"),
		Opt::flag(Arg::Scan, &["-s", "--scan"])
			.help_text("Recursively scan path as a directory"),
	]);
	match OPTIONS.parse_easy(|program_name, id, _opt, _name, arg|
	{
		match id
		{
			Arg::Help =>
			{
				OPTIONS.print_full_help(program_name);
				return Ok(ParseControl::Quit);
			}
			Arg::Path => { path.push(arg); }
			Arg::Scan => { scan = true; }
		}
		Ok(ParseControl::Continue)
	}) {
		ParseResult::ContinueSuccess => ParseArgsResult::Continue(scan, path),
		ParseResult::ExitSuccess => ParseArgsResult::Exit(ExitCode::SUCCESS),
		ParseResult::ExitError => ParseArgsResult::Exit(ExitCode::FAILURE),
	}
}

fn walk(dir: &Path, cb: &dyn Fn(&str, &str, &Path)) -> Result<(), Error>
{
	for node in fs::read_dir(dir)?
	{
		if let Ok(node) = node
		{
			let path = node.path();
			if path.is_dir()
			{
				walk(&path, cb)?;
			}
			else
			{
				cb(
					&dir.to_string_lossy(),
					&*path.file_name().unwrap().to_string_lossy(),
					&path);
			}
		}
	}
	Ok(())
}

fn main() -> ExitCode
{
	let (scan, path) = match parseArgs()
	{
		ParseArgsResult::Continue(scan, path) => (scan, path),
		ParseArgsResult::Exit(code) => { return code; }
	};
	if scan
	{
		if !path.is_dir()
		{
			eprintln!("Error: \"{}\" is not a directory", path.display());
			return ExitCode::FAILURE;
		}
		if let Err(err) = walk(&path, &|dirname, filename, path|
		{
			match LBM::read(&path)
			{
				Ok(lbm) => { println!("{dirname}/{filename}: {}", lbm.shortInfo()) }
				Err(err) => { println!("{dirname}/{filename}: {:?}", err) }
			};
		})
		{
			eprintln!("Error during scan: {:?}", err);
			return ExitCode::FAILURE;
		}
	}
	else
	{
		let lbm = match LBM::read(&path)
		{
			Ok(lbm) => lbm,
			Err(err) =>
			{
				eprintln!("Error: failed to open file \"{}\": {:?}", path.display(), err);
				return ExitCode::FAILURE;
			}
		};
		if let Some(name) = path.file_name().unwrap().to_str()
		{
			println!("File: {name}");
		}
		print!("{}", lbm.fullInfo());
	}
	ExitCode::SUCCESS
}
