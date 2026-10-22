// Copyright (C) 2026 John KNIPPER <code@jkn.me>.
// Licensed under the AROS Public License (APL), Version 1.1.
//! Guest probe for the matching batch-capable AROSTCP provider and C adapter.
use std::ffi::{c_int, c_short, c_uint};
use std::net::{Shutdown, TcpListener, TcpStream, UdpSocket};
use std::os::fd::AsRawFd;
use std::time::{Duration, Instant};
#[repr(C)]
struct PollFd { fd: c_int, events: c_short, revents: c_short }
unsafe extern "C" {
    fn aros_rs_poll(fds: *mut PollFd, count: c_uint, timeout: c_int) -> c_int;
    fn pipe(fds: *mut c_int) -> c_int;
    fn close(fd: c_int) -> c_int;
}
fn poll(fds: &mut [PollFd], timeout: i32) -> i32 {
    unsafe { aros_rs_poll(fds.as_mut_ptr(), fds.len() as u32, timeout) }
}
fn entry(fd: i32, events: i16) -> PollFd { PollFd { fd, events, revents: -1 } }
fn main() {
    assert_eq!(std::mem::size_of::<PollFd>(), 8);
    assert_eq!(std::mem::align_of::<PollFd>(), 4);
    assert_eq!(std::mem::offset_of!(PollFd, events), 4);
    assert_eq!(std::mem::offset_of!(PollFd, revents), 6);
    let recv = UdpSocket::bind("127.0.0.1:0").unwrap();
    let idle = UdpSocket::bind("127.0.0.1:0").unwrap();
    let send = UdpSocket::bind("127.0.0.1:0").unwrap();
    send.send_to(b"batch", recv.local_addr().unwrap()).unwrap();
    let mut fds = [entry(recv.as_raw_fd(), 1), entry(idle.as_raw_fd(), 1), entry(-1, -1)];
    assert_eq!(poll(&mut fds, 1000), 1);
    assert_eq!([fds[0].revents, fds[1].revents, fds[2].revents], [1, 0, 0]);
    println!("AROS_POLL case=batch-udp result=PASS");
    let mut mixed = [entry(-1, 1), entry(i32::MAX, 0), entry(recv.as_raw_fd(), 1)];
    assert_eq!(poll(&mut mixed, -1), 2);
    assert_eq!([mixed[0].revents, mixed[1].revents, mixed[2].revents], [0, 0x20, 1]);
    println!("AROS_POLL case=mixed-invalid result=PASS");
    let start = Instant::now();
    assert_eq!(poll(&mut [], 30), 0);
    assert!(start.elapsed() >= Duration::from_millis(20));
    // Existing data with no requested IN must not be invented as readiness.
    let mut none = [entry(recv.as_raw_fd(), 0)];
    let start = Instant::now();
    assert_eq!(poll(&mut none, 30), 0);
    assert_eq!(none[0].revents, 0);
    assert!(start.elapsed() >= Duration::from_millis(20));
    println!("AROS_POLL case=empty-and-unrequested-deadline result=PASS");
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    let stream = TcpStream::connect(listener.local_addr().unwrap()).unwrap();
    let (_peer, _) = listener.accept().unwrap();
    stream.shutdown(Shutdown::Both).unwrap();
    let mut hup = [entry(stream.as_raw_fd(), 1 | 4)];
    assert_eq!(poll(&mut hup, 0), 1);
    assert!(hup[0].revents & 0x10 != 0);
    assert_eq!(hup[0].revents & 4, 0);
    println!("AROS_POLL case=stream-full-hup result=PASS");
    // Connected UDP to a freshly released loopback port yields an ICMP error.
    // Poll observes it twice; only take_error is allowed to consume SO_ERROR.
    let error_socket = UdpSocket::bind("127.0.0.1:0").unwrap();
    let reserved = UdpSocket::bind("127.0.0.1:0").unwrap();
    let closed_port = reserved.local_addr().unwrap();
    drop(reserved);
    error_socket.connect(closed_port).unwrap();
    error_socket.send(b"refused").unwrap();
    let mut error = [entry(error_socket.as_raw_fd(), 0)];
    assert_eq!(poll(&mut error, 1000), 1);
    assert_eq!(error[0].revents, 8);
    assert_eq!(poll(&mut error, 0), 1);
    assert_eq!(error[0].revents, 8);
    assert_eq!(error_socket.take_error().unwrap().unwrap().raw_os_error(), Some(61));
    assert_eq!(poll(&mut error, 0), 0);
    println!("AROS_POLL case=pending-error-preserved result=PASS");
    let mut pipes = [-1; 2];
    assert_eq!(unsafe { pipe(pipes.as_mut_ptr()) }, 0);
    let mut unsupported = [entry(pipes[0], 1)];
    assert_eq!(poll(&mut unsupported, 0), -1);
    // AROS EOPNOTSUPP=45: explicit socket-only scope, never POLLNVAL for a pipe.
    assert_eq!(std::io::Error::last_os_error().raw_os_error(), Some(45));
    assert_eq!(unsafe { close(pipes[0]) }, 0);
    assert_eq!(unsafe { close(pipes[1]) }, 0);
    println!("AROS_POLL case=pipe-explicitly-unsupported result=PASS");
    println!("AROS_POLL result=PASS cases=6");
}
