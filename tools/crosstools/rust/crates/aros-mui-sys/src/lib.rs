#![cfg(target_os = "aros")]
#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case, dead_code)]

include!("bindings.rs");

mod macros;
pub use macros::TagValue;

mod dsl;
pub use dsl::*;

const _: () = assert!(core::mem::size_of::<TagItem>() == 16);
