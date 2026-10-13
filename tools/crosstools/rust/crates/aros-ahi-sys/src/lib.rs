//! Raw bindings to `ahi.c`: playback through ahi.device.
//!
//! Empty on every other target.
#![cfg(target_os = "aros")]
#![allow(non_camel_case_types)]

use std::ffi::{c_int, c_uint, c_void};

// Sample types, devices/ahi.h.
pub const AHIST_M16S: c_uint = 1;
pub const AHIST_S16S: c_uint = 3;

#[repr(C)]
pub struct aros_ahi {
    _private: [u8; 0],
}

extern "C" {
    /// Opens the default AHI unit. Call every function from the task that opened it.
    pub fn aros_ahi_open() -> *mut aros_ahi;
    /// Queues `bytes` of `sample_type` samples at `rate` Hz, then waits for the
    /// previous buffer to finish. Returns 0 or that buffer's AHI error.
    pub fn aros_ahi_write(a: *mut aros_ahi, data: *const c_void, bytes: c_uint, sample_type: c_uint, rate: c_uint) -> c_int;
    /// Waits for the queued buffers to play out.
    pub fn aros_ahi_drain(a: *mut aros_ahi);
    pub fn aros_ahi_close(a: *mut aros_ahi);
}
