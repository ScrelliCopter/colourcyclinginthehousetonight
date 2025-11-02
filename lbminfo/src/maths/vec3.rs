#[derive(Default, PartialEq, Debug, Copy, Clone)]
pub(crate) struct Vec3<T> { pub(crate) x: T, pub(crate) y: T, pub(crate) z: T }

impl<T> Vec3<T>
{
	#[inline(always)]
	pub(crate) fn new(x: T, y: T, z: T) -> Self { Self { x, y, z } }
}

pub(crate) type Vec3i = Vec3<i32>;
