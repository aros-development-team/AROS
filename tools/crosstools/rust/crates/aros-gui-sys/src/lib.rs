//! Raw bindings to the C glue for Intuition windows, input and signals on
//! AROS. The C half is `glue.c`; keep the structures in step with `glue.h`.
//!
//! Empty on every other target.
#![cfg(target_os = "aros")]
#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_int, c_void};

pub const AROS_GUI_WINDOW_RESIZABLE: u32 = 1 << 0;
pub const AROS_GUI_WINDOW_BORDERLESS: u32 = 1 << 1;
pub const AROS_GUI_WINDOW_ACTIVATE: u32 = 1 << 2;

pub const AROS_GUI_FR_SAVE: c_int = 1;
pub const AROS_GUI_FR_DRAWERS: c_int = 2;

// IDCMP classes, intuition/intuition.h.
pub const IDCMP_NEWSIZE: u32 = 1 << 1;
pub const IDCMP_MOUSEBUTTONS: u32 = 1 << 3;
pub const IDCMP_MOUSEMOVE: u32 = 1 << 4;
pub const IDCMP_DELTAMOVE: u32 = 1 << 20;
pub const IDCMP_CLOSEWINDOW: u32 = 1 << 9;
pub const IDCMP_RAWKEY: u32 = 1 << 10;
pub const IDCMP_ACTIVEWINDOW: u32 = 1 << 18;
pub const IDCMP_INACTIVEWINDOW: u32 = 1 << 19;
pub const IDCMP_CHANGEWINDOW: u32 = 1 << 25;

// Input event codes and qualifiers, devices/inputevent.h.
pub const IECODE_UP_PREFIX: u16 = 0x80;
pub const IECODE_LBUTTON: u16 = 0x68;
pub const IECODE_RBUTTON: u16 = 0x69;
pub const IECODE_MBUTTON: u16 = 0x6A;

pub const IEQUALIFIER_LSHIFT: u16 = 1 << 0;
pub const IEQUALIFIER_RSHIFT: u16 = 1 << 1;
pub const IEQUALIFIER_CAPSLOCK: u16 = 1 << 2;
pub const IEQUALIFIER_CONTROL: u16 = 1 << 3;
pub const IEQUALIFIER_LALT: u16 = 1 << 4;
pub const IEQUALIFIER_RALT: u16 = 1 << 5;
pub const IEQUALIFIER_LCOMMAND: u16 = 1 << 6;
pub const IEQUALIFIER_RCOMMAND: u16 = 1 << 7;
pub const IEQUALIFIER_NUMERICPAD: u16 = 1 << 8;
pub const IEQUALIFIER_REPEAT: u16 = 1 << 9;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct aros_gui_window_attrs {
    pub left: i32,
    pub top: i32,
    pub inner_width: i32,
    pub inner_height: i32,
    pub min_width: i32,
    pub min_height: i32,
    pub flags: u32,
    pub title: *const c_char,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct aros_gui_window_info {
    pub handle: usize,
    pub left: i32,
    pub top: i32,
    pub width: i32,
    pub height: i32,
    pub inner_width: i32,
    pub inner_height: i32,
    pub border_left: i32,
    pub border_top: i32,
    pub sigmask: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct aros_gui_message {
    pub class: u32,
    pub code: u16,
    pub qualifier: u16,
    pub mouse_x: i32,
    pub mouse_y: i32,
}

extern "C" {
    pub fn aros_gui_screen_size(width: *mut i32, height: *mut i32) -> c_int;
    pub fn aros_gui_screen_palette(rgb: *mut u32, count: c_int) -> c_int;

    pub fn aros_gui_window_open(
        attrs: *const aros_gui_window_attrs,
        out: *mut aros_gui_window_info,
    ) -> c_int;
    pub fn aros_gui_window_close(window: usize);
    pub fn aros_gui_window_info(window: usize, out: *mut aros_gui_window_info);
    pub fn aros_gui_window_set_title(window: usize, title: *const c_char) -> c_int;
    pub fn aros_gui_window_set_inner_size(window: usize, width: i32, height: i32);
    pub fn aros_gui_window_set_position(window: usize, left: i32, top: i32);
    pub fn aros_gui_window_set_min_size(window: usize, width: i32, height: i32);
    pub fn aros_gui_window_set_pointer_visible(window: usize, visible: c_int);
    pub fn aros_gui_window_focus(window: usize);
    pub fn aros_gui_window_is_active(window: usize) -> c_int;
    pub fn aros_gui_window_next_message(window: usize, out: *mut aros_gui_message) -> c_int;
    pub fn aros_gui_window_set_delta_move(window: usize, on: c_int);
    pub fn aros_gui_window_delta_move(window: usize) -> c_int;

    pub fn aros_gui_map_rawkey(code: u16, qualifier: u16, buf: *mut c_char, len: c_int) -> c_int;

    pub fn aros_gui_task_self() -> *mut c_void;
    pub fn aros_gui_signal_alloc() -> c_int;
    pub fn aros_gui_signal_free(bit: c_int);
    pub fn aros_gui_signal_send(task: *mut c_void, mask: u32);
    pub fn aros_gui_wait(mask: u32, timeout_us: i64, timed_out: *mut c_int) -> u32;
    pub fn aros_gui_wait_cleanup();

    pub fn aros_gui_clip_write(text: *const u8, len: i32) -> c_int;
    pub fn aros_gui_clip_read(len: *mut i32) -> *mut u8;
    pub fn aros_gui_clip_free(text: *mut u8);

    pub fn aros_gui_file_request(
        window: usize,
        title: *const c_char,
        drawer: *const c_char,
        file: *const c_char,
        pattern: *const c_char,
        flags: c_int,
    ) -> *mut c_char;
    pub fn aros_gui_message(
        window: usize,
        title: *const c_char,
        text: *const c_char,
        buttons: *const c_char,
    ) -> c_int;
    pub fn aros_gui_string_free(text: *mut c_char);

    pub fn aros_gui_gl_create(window: usize, alpha: c_int, depth: c_int, stencil: c_int) -> usize;
    pub fn aros_gui_gl_create_profile(
        window: usize,
        alpha: c_int,
        depth: c_int,
        stencil: c_int,
        core: c_int,
        major: c_int,
        minor: c_int,
    ) -> usize;
    pub fn aros_gui_gl_destroy(ctx: usize);
    pub fn aros_gui_gl_make_current(ctx: usize);
    pub fn aros_gui_gl_current() -> usize;
    pub fn aros_gui_gl_swap_buffers(ctx: usize);
    pub fn aros_gui_gl_set_window(ctx: usize, window: usize);
    pub fn aros_gui_gl_proc(name: *const c_char) -> *const c_void;
}
