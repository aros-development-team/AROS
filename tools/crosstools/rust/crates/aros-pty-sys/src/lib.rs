//! Raw bindings to the AROS pseudo console in `pty.c`; see `pty.h`.
//!
//! Empty on every other target.
#![cfg(target_os = "aros")]
#![allow(non_camel_case_types)]

use std::ffi::c_char;

#[repr(C)]
pub struct aros_pty {
    _private: [u8; 0],
}

extern "C" {
    pub fn aros_pty_spawn(command: *const c_char, cols: i32, rows: i32) -> *mut aros_pty;
    pub fn aros_pty_read(pty: *mut aros_pty, buf: *mut u8, len: i32) -> i32;
    pub fn aros_pty_write(pty: *mut aros_pty, buf: *const u8, len: i32);
    pub fn aros_pty_resize(pty: *mut aros_pty, cols: i32, rows: i32);
    pub fn aros_pty_close(pty: *mut aros_pty);
    pub fn aros_pty_free(pty: *mut aros_pty);
}
