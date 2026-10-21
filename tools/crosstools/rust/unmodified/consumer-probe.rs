// Copyright (C) 2026 John KNIPPER <code@jkn.me>
// Licensed under the AROS Public License (APL), Version 1.1

use std::net::{Ipv4Addr, UdpSocket};

unsafe extern "C" {
    fn aros_rs_consumer_trace(event: *const std::ffi::c_char);
    fn aros_rs_consumer_runtime() -> u32;
}
fn trace(event: &std::ffi::CStr) { unsafe { aros_rs_consumer_trace(event.as_ptr()); } }

fn main() {
    trace(c"main");
    let hook = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| { let text = std::ffi::CString::new(info.to_string()).unwrap(); trace(&text); hook(info); }));
    let runtime = unsafe { aros_rs_consumer_runtime() };
    assert!(runtime >= 41);
    println!("[RUSTAPP_RUNTIME] posix-version={} revision={}", runtime >> 16, runtime & 65535);
    let arguments: Vec<_> = std::env::args().collect();
    assert_eq!(arguments.len(), 3);
    assert_eq!(&arguments[1..], ["alpha", "beta"]);
    let socket = UdpSocket::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
    socket.set_multicast_ttl_v4(3).unwrap();
    assert_eq!(socket.multicast_ttl_v4().unwrap(), 3);
    // Use the application's libc alongside std's internal libc dependency.
    let fd = unsafe { libc::socket(libc::AF_INET, libc::SOCK_DGRAM, 0) };
    assert!(fd >= 0);
    assert_eq!(unsafe { libc::close(fd) }, 0);
    // Generic poll is explicitly unsupported by this native interface.
    assert_eq!(unsafe { libc::poll(std::ptr::null_mut(), 0, 0) }, -1);
    assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
    assert_eq!(unsafe { libc::if_nametoindex(c"unavailable".as_ptr()) }, 0);
    assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
    let mut template = *b"RAM:unsupported-XXXXXX\0";
    assert!(unsafe { libc::mkdtemp(template.as_mut_ptr().cast()) }.is_null());
    assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
    assert_eq!(&template, b"RAM:unsupported-XXXXXX\0");
    // FileExt must preserve the cursor across duplicated descriptors and
    // concurrent positioned operations, using the native shared-fcb lock.
    use std::fs::OpenOptions;
    use std::io::{Read, Seek, SeekFrom, Write};
    use std::os::unix::fs::FileExt;
    trace(c"network-done file-open");
    let path = "RAM:rust-positioned-probe";
    let mut file = OpenOptions::new().read(true).write(true).create(true)
        .truncate(true).open(path).unwrap();
    trace(c"file-write");
    file.write_all(b"0123456789abcdef").unwrap();
    trace(c"file-seek");
    file.seek(SeekFrom::Start(3)).unwrap();
    trace(c"file-clone");
    let clone = file.try_clone().unwrap();
    trace(c"file-write-at");
    use std::os::fd::AsRawFd;
    assert_ne!(unsafe { libc::fcntl(clone.as_raw_fd(), libc::F_GETFD) } & libc::FD_CLOEXEC, 0);
    let plain = unsafe { libc::fcntl(file.as_raw_fd(), libc::F_DUPFD, 20) };
    assert!(plain >= 20);
    assert_eq!(unsafe { libc::fcntl(plain, libc::F_GETFD) } & libc::FD_CLOEXEC, 0);
    assert_eq!(unsafe { libc::close(plain) }, 0);
    assert_eq!(clone.write_at(b"XY", 8).unwrap(), 2);
    assert_eq!(file.stream_position().unwrap(), 3);
    trace(c"file-read-at");
    let mut bytes = [0; 4];
    assert_eq!(clone.read_at(&mut bytes, 7).unwrap(), 4);
    assert_eq!(&bytes, b"7XYa");
    assert_eq!(file.stream_position().unwrap(), 3);
    assert_eq!(clone.read_at(&mut bytes, 100).unwrap(), 0);
    assert_eq!(file.metadata().unwrap().len(), 16);
    trace(c"file-thread-spawn");
    let reader = std::thread::spawn(move || {
        // The application libc and std read the same native task-local errno.
        assert_eq!(unsafe { libc::fork() }, -1);
        assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
        assert_eq!(unsafe { libc::poll(std::ptr::null_mut(), 0, 0) }, -1);
        assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
        assert_eq!(unsafe { libc::mmap(std::ptr::null_mut(), 4096, 0, 0, -1, 0) }, (-1isize) as *mut libc::c_void);
        assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(libc::ENOSYS));
        for _ in 0..128 {
            let mut bytes = [0; 4];
            assert_eq!(clone.read_at(&mut bytes, 7).unwrap(), 4);
            assert_eq!(&bytes, b"7XYa");
        }
    });
    for _ in 0..128 {
        let mut bytes = [0; 4];
        assert_eq!(file.read_at(&mut bytes, 0).unwrap(), 4);
        assert_eq!(&bytes, b"0123");
    }
    trace(c"file-thread-join");
    reader.join().unwrap();
    assert_eq!(file.stream_position().unwrap(), 3);
    file.read_exact(&mut bytes).unwrap();
    assert_eq!(&bytes, b"3456");
    trace(c"file-cleanup");
    drop(file);
    std::fs::remove_file(path).unwrap();
    println!("[RUSTFILE] result=PASS scope=ram-positioned-io concurrent-reads=256");
    println!("[RUSTAPP] result=PASS mode=precompiled-std args=3 port={}",
             socket.local_addr().unwrap().port());
}
