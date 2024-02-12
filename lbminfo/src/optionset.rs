/* optionset.rs - Just enough OptionSet to make dealing with bitfields not painful
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

//FIXME: This is pretty stanky

macro_rules! optionset
{
	($(#[$d:meta])* $vis:vis struct $name:ident: $t:ty { $($(#[$attr:meta])* $key:ident = $val:expr;)* }) =>
	{
		$(#[$d])*
		#[derive(Copy, Clone, PartialEq, Eq)]
		$vis struct $name($t);
		impl $name
		{
			$(
				$(#[$attr])*
				$vis const $key: $name = $name($val);
			)*

			#[inline]
			#[allow(dead_code)]
			//pub(crate) fn contains(&self, f: Self) -> bool
			pub(crate) fn contains<F: std::borrow::Borrow<$name>>(&self, f: F) -> bool
			{
				//if self.0 & f.0 == f.0 { true } else { false }
				if self.0 & f.borrow().0 == f.borrow().0 { true } else { false }
			}

			const FLAGS: &'static [Self] = &[$(Self::$key),*];
		}

		impl From<$name> for $t
		{
			#[inline]
			fn from(value: $name) -> Self { value.0 }
		}

		crate::optionset::_optionset_bitop!(impl BitAnd for $name, bitand { & });
		crate::optionset::_optionset_bitop!(impl BitOr for $name, bitor { | });
		crate::optionset::_optionset_bitop!(impl BitXor for $name, bitxor { ^ });
	};
}
pub(crate) use optionset;

//TODO: Feex
/*
macro_rules! optionset_default_display
{
	($t:ident) =>
	{
		impl std::fmt::Display for $t
		{
			fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result
			{
				for (idx, x) in Self::FLAGS.iter()
					.filter(|&x| self.contains(x)).enumerate()
				{
					if idx != 0 { write!(f, " | ")?; }
					write!(f, "{:?}", x)?;
				}
				Ok(())
			}
		}
	};
}
pub(crate) use optionset_default_display;
 */

macro_rules! optionset_custom_display
{
	(impl Display for $t:ident { $($key:ident => $value:expr),* $(,)* }) =>
	{
		impl $t
		{
			fn _internal_to_string(&self) -> &'static str
			{
				match *self
				{
					$(Self::$key => $value),*,
					_ => { panic!("Shouldn't happen") }
				}
			}
		}
		impl std::fmt::Display for $t
		{
			fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result
			{
				if self == &$t(0) || Self::FLAGS.contains(self)
				{
					write!(f, "{}", self._internal_to_string())
				}
				else
				{
					write!(f, "{}", Self::FLAGS.iter()
						.filter(|&x| self.contains(x))
						.map(|x| x._internal_to_string())
						.collect::<Vec<_>>().join(" | "))
				}
			}
		}
	}
}
pub(crate) use optionset_custom_display;

macro_rules! _optionset_bitop
{
	(impl $imp:ident for $iden:ident, $func:ident { $e:tt }) =>
	{
		impl std::ops::$imp for $iden
		{
			type Output = $iden;
			#[inline(always)]
			fn $func(self, rhs: $iden) -> Self::Output { Self(self.0 $e rhs.0) }
		}
		crate::optionset::forward_ref_binop!(impl $imp, $func for $iden, $iden);
	};
}
pub(crate) use _optionset_bitop;

macro_rules! forward_ref_binop
{
	(impl $imp:ident, $method:ident for $t:ty, $u:ty) =>
	{
		impl<'a> std::ops::$imp<$u> for &'a $t
		{
			type Output = <$t as std::ops::$imp<$u>>::Output;
			#[inline]
			fn $method(self, rhs: $u) -> <$t as std::ops::$imp<$u>>::Output
			{
				std::ops::$imp::$method(*self, rhs)
			}
		}

		impl<'a> std::ops::$imp<&'a $u> for $t
		{
			type Output = <$t as std::ops::$imp<$u>>::Output;
			#[inline]
			fn $method(self, rhs: &'a $u) -> <$t as std::ops::$imp<$u>>::Output
			{
				std::ops::$imp::$method(self, *rhs)
			}
		}

		impl<'a, 'b> std::ops::$imp<&'a $u> for &'b $t
		{
			type Output = <$t as std::ops::$imp<$u>>::Output;
			#[inline]
			fn $method(self, rhs: &'a $u) -> <$t as std::ops::$imp<$u>>::Output
			{
				std::ops::$imp::$method(*self, *rhs)
			}
		}
	}
}
pub(crate) use forward_ref_binop;
