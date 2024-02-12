/* main.rs
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

#![allow(non_snake_case)]

mod chunk;
mod lbm;
mod fsext;
mod info;
mod check;
mod optionset;

use std::{env, fs, io};
use std::path::{Path};
use std::process::exit;
use crate::info::LBMInfo;
use crate::lbm::LBM;


fn parseArgs() -> Result<(bool, String), String>
{
	let args: Vec<String> = env::args().collect();
	match args.len()
	{
		2 => { Ok((false, args[1].trim().parse().unwrap())) }
		3 =>
		{
			let arg1 = args[1].trim();
			if !arg1.starts_with('-') { return Err(String::from("Too many arguments")) }
			if !["-s", "--scan"].contains(&arg1) { return Err(format!("Unrecognised option `{}`", arg1)) }
			Ok((true, args[2].trim().parse().unwrap()))
		}
		0..=1 => { Err(String::from("Not enough arguments")) }
		_ => { Err(String::from("Too many arguments")) }
	}
}

fn walk(dir: &Path, cb: &dyn Fn(&str, &str, &Path)) -> Result<(), io::Error>
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

fn main()
{
	let (scan, pathName) = match parseArgs()
	{
		Ok(tuple) => tuple,
		Err(err) =>
		{
			eprintln!("Error: {err}");
			exit(1);
		}
	};

	let path = Path::new(&pathName);
	if scan
	{
		if !path.is_dir()
		{
			eprintln!("Error: \"{}\" is not a directory", path.display());
			exit(1);
		}
		walk(&path, &|dirname, filename, path|
		{
			match LBM::read(&path)
			{
				Ok(lbm) => { println!("{dirname}/{filename}: {}", lbm.shortInfo()) }
				Err(err) => { println!("{dirname}/{filename}: {:?}", err) }
			};
		}).unwrap_or_else(|err|
		{
			eprintln!("Error during scan: {:?}", err);
			exit(1);
		})
	}
	else
	{
		match LBM::read(&path)
		{
			Ok(lbm) =>
			{
				if let Some(name) = path.file_name().unwrap().to_str() { println!("File: {name}"); }
				print!("{}", lbm.fullInfo());
			}
			Err(err) =>
			{
				eprintln!("Error: failed to open file \"{}\": {:?}", path.display(), err);
				exit(1);
			}
		}
	}
}
