// Copyright (C) 2026 John KNIPPER <code@jkn.me>.
// Distributed under the AROS Public License 1.1.
use libc::{cmsghdr, msghdr, CMSG_DATA, CMSG_FIRSTHDR, CMSG_LEN, CMSG_NXTHDR, CMSG_SPACE};
#[repr(align(16))]
struct Buffer([u8; 128]);
pub fn checks() -> u32 {
    let mut count = 0;
    macro_rules! check { ($e:expr) => {{ assert!($e, "{}", stringify!($e)); count += 1; }}; }
    unsafe {
        check!(core::mem::size_of::<cmsghdr>() == 12);
        check!(CMSG_SPACE(0) == 16); check!(CMSG_LEN(0) == 16);
        check!(CMSG_SPACE(1) == 32); check!(CMSG_LEN(1) == 17);
        check!(CMSG_SPACE(16) == 32); check!(CMSG_SPACE(17) == 48);
        let mut data = Buffer([0; 128]);
        let base = core::hint::black_box(data.0.as_mut_ptr());
        let header = base.cast::<cmsghdr>();
        let mut msg: msghdr = core::mem::zeroed();
        msg.msg_control = base.cast();
        for length in [0, 1, 11] {
            msg.msg_controllen = length;
            check!(CMSG_FIRSTHDR(core::hint::black_box(&msg)).is_null());
            check!(CMSG_NXTHDR(core::hint::black_box(&msg), core::ptr::null()).is_null());
        }
        msg.msg_controllen = 12;
        check!(CMSG_FIRSTHDR(core::hint::black_box(&msg)) == header);
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), core::ptr::null()) == header);
        (*header).cmsg_len = CMSG_LEN(1) as _;
        check!(CMSG_DATA(header) == base.add(16));
        CMSG_DATA(header).write(0x5a);
        check!(data.0[16] == 0x5a);
        msg.msg_controllen = 47;
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), header).is_null());
        msg.msg_controllen = 48;
        let second = base.add(32).cast::<cmsghdr>();
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), header) == second);
        (*second).cmsg_len = CMSG_LEN(0) as _;
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), second).is_null());
        msg.msg_controllen = 64;
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), second) == base.add(48).cast());
        (*header).cmsg_len = u32::MAX;
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), header).is_null());
        msg.msg_control = (usize::MAX - 7) as *mut _;
        check!(CMSG_NXTHDR(core::hint::black_box(&msg), header).is_null());
        msg.msg_control = core::ptr::null_mut();
        check!(CMSG_FIRSTHDR(core::hint::black_box(&msg)).is_null());
    }
    count
}
