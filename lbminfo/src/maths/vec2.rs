#[derive(Default, PartialEq, Debug, Copy, Clone)]
pub(crate) struct Vec2<T> { pub(crate) x: T, pub(crate) y: T }

impl<T> Vec2<T>
{
	#[inline(always)]
	pub(crate) fn new(x: T, y: T) -> Self { Self { x, y } }
}
