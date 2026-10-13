use core::ffi::CStr;
use crate::IPTR;

pub trait TagValue {
    fn into_iptr(self) -> IPTR;
}

impl TagValue for i32  { fn into_iptr(self) -> IPTR { self as IPTR } } // sign-extend, som C
impl TagValue for u32  { fn into_iptr(self) -> IPTR { self as IPTR } }
impl TagValue for bool { fn into_iptr(self) -> IPTR { self as IPTR } }
impl TagValue for IPTR { fn into_iptr(self) -> IPTR { self } }
impl<T> TagValue for *mut T   { fn into_iptr(self) -> IPTR { self as IPTR } }
impl<T> TagValue for *const T { fn into_iptr(self) -> IPTR { self as IPTR } }
impl TagValue for &CStr { fn into_iptr(self) -> IPTR { self.as_ptr() as IPTR } }

#[macro_export]
macro_rules! tags {
    ($($tag:expr => $val:expr),* $(,)?) => {
        [
            $(
                $crate::TagItem {
                    ti_Tag: $tag as $crate::Tag,
                    ti_Data: $crate::TagValue::into_iptr($val),
                },
            )*
            $crate::TagItem { ti_Tag: $crate::TAG_DONE as $crate::Tag, ti_Data: 0 },
        ]
    };
}

// obj!(Window { tag => value, ... }) creates a MUIC_<class> object; values
// can be nested obj!() calls.
#[macro_export]
macro_rules! obj {
    (VGroup { $($t:tt)* }) => { $crate::obj!(@new Group, $($t)*) };
    (HGroup { $($t:tt)* }) => { $crate::obj!(@new Group, $crate::MUIA_Group_Horiz => true, $($t)*) };
    ($class:ident { $($t:tt)* }) => { $crate::obj!(@new $class, $($t)*) };
    (@new $class:ident, $($t:tt)*) => {
        $crate::MUI_NewObjectA($crate::class::$class.as_ptr(), $crate::tags![$($t)*].as_mut_ptr())
    };
}

#[macro_export]
macro_rules! method {
    ($id:expr $(, $arg:expr)* $(,)?) => {
        [($id as $crate::IPTR) $(, $crate::TagValue::into_iptr($arg))*]
    };
}
