// Copyright (C) 2026 John KNIPPER <code@jkn.me>
// Licensed under the AROS Public License (APL) Version 1.1

use std::io::{self, Read, Write};
use std::net::{Ipv4Addr, TcpListener, TcpStream, UdpSocket};
use std::thread;
use std::time::Duration;

#[cfg(target_os = "aros")]
fn checkpoint(event: &std::ffi::CStr) {
    unsafe extern "C" { fn aros_rs_probe_event(event: *const std::ffi::c_char); }
    unsafe { aros_rs_probe_event(event.as_ptr()) }
}
#[cfg(not(target_os = "aros"))]
fn checkpoint(_: &std::ffi::CStr) {}

const DEADLINE: Duration = Duration::from_secs(3);

fn require(condition: bool, message: &str) -> io::Result<()> {
    if condition { Ok(()) } else { Err(io::Error::other(message)) }
}

// Test the actual resolver conversion, even though std optimizes numeric
// ToSocketAddrs inputs without calling getaddrinfo. This is the Rust-side ABI
// of resolver.h, deliberately distinct from native AROS's LP64 addrinfo.
#[cfg(target_os = "aros")]
fn resolver() -> io::Result<()> {
    use std::ffi::{c_char, c_int, c_void};
    use std::ptr;
    #[repr(C)]
    struct AddrInfo {
        flags: c_int, family: c_int, socktype: c_int, protocol: c_int,
        length: u32, canonical: *mut c_char, address: *mut c_void, next: *mut AddrInfo,
    }
    unsafe extern "C" {
        #[link_name = "aros_rs_getaddrinfo"]
        fn lookup(node: *const c_char, service: *const c_char, hints: *const AddrInfo,
                  result: *mut *mut AddrInfo) -> c_int;
        #[link_name = "aros_rs_freeaddrinfo"]
        fn dispose(result: *mut AddrInfo);
    }
    struct Owned(*mut AddrInfo);
    impl Drop for Owned { fn drop(&mut self) { unsafe { dispose(self.0) } } }
    let hints = AddrInfo { flags: 0, family: 2, socktype: 1, protocol: 0,
        length: 0, canonical: ptr::null_mut(), address: ptr::null_mut(), next: ptr::null_mut() };
    let mut result = ptr::null_mut();
    let code = unsafe { lookup(c"127.0.0.1".as_ptr(), c"80".as_ptr(), &hints, &mut result) };
    let owned = Owned(result);
    require(code == 0 && !owned.0.is_null(), "numeric native resolver failed")?;
    let entry = unsafe { &*owned.0 };
    require(entry.family == 2 && entry.length == 16 && !entry.address.is_null(),
            "resolver returned an invalid IPv4 record")?;
    // The per-call native base is closed already; another thread frees the list.
    let address = owned.0 as usize;
    std::mem::forget(owned);
    thread::spawn(move || drop(Owned(address as *mut AddrInfo))).join()
        .map_err(|_| io::Error::other("resolver disposal thread failed"))?;
    Ok(())
}

#[cfg(not(target_os = "aros"))]
fn resolver() -> io::Result<()> { Ok(()) } // Native bridge is tested only on AROS.

fn exchange() -> io::Result<()> {
    let listener = thread::spawn(|| TcpListener::bind((Ipv4Addr::LOCALHOST, 0)))
        .join().map_err(|_| io::Error::other("listener creator failed"))??;
    let address = listener.local_addr()?;
    listener.set_nonblocking(false)?;
    let client = thread::spawn(move || TcpStream::connect_timeout(&address, DEADLINE))
        .join().map_err(|_| io::Error::other("stream creator failed"))??;
    let (mut server, peer) = listener.accept()?;
    require(peer.ip().is_loopback(), "unexpected TCP peer")?;
    server.set_read_timeout(Some(DEADLINE))?;
    client.set_read_timeout(Some(DEADLINE))?;
    client.set_write_timeout(Some(DEADLINE))?;
    client.set_nodelay(true)?;
    require(client.nodelay()?, "TCP_NODELAY was not retained")?;
    let mut clone = client.try_clone()?;
    drop(client);
    clone.write_all(b"aros")?;
    let mut bytes = [0; 4];
    server.read_exact(&mut bytes)?;
    require(bytes == *b"aros", "TCP payload mismatch")?;
    server.write_all(b"rust")?;
    clone.read_exact(&mut bytes)?;
    require(bytes == *b"rust", "TCP reply mismatch")?;
    drop(server);
    drop(clone);
    drop(listener);
    // Writable readiness alone is not a successful connection. AROS must
    // inspect SO_ERROR when the peer port has no listener.
    require(TcpStream::connect_timeout(&address, DEADLINE).is_err(),
            "closed TCP port reported a successful connection")
}

fn datagrams() -> io::Result<()> {
    let receiver = UdpSocket::bind((Ipv4Addr::LOCALHOST, 0))?;
    receiver.set_read_timeout(Some(DEADLINE))?;
    let address = receiver.local_addr()?;
    let sender = thread::spawn(|| UdpSocket::bind((Ipv4Addr::LOCALHOST, 0)))
        .join().map_err(|_| io::Error::other("UDP creator failed"))??;
    checkpoint(c"[RUSTNET_RAW] udp=options");
    sender.set_multicast_loop_v4(true)?;
    sender.set_multicast_ttl_v4(3)?;
    require(sender.multicast_loop_v4()? && sender.multicast_ttl_v4()? == 3,
            "IPv4 multicast option width/value mismatch")?;
    checkpoint(c"[RUSTNET_RAW] udp=clone");
    let clone = sender.try_clone()?;
    drop(sender);
    checkpoint(c"[RUSTNET_RAW] udp=send");
    require(clone.send_to(b"aros", address)? == 4, "short UDP send")?;
    let mut bytes = [0; 16];
    checkpoint(c"[RUSTNET_RAW] udp=receive");
    let (length, peer) = receiver.recv_from(&mut bytes)?;
    require(length == 4 && &bytes[..length] == b"aros" && peer.ip().is_loopback(),
            "UDP payload/address mismatch")?;
    checkpoint(c"[RUSTNET_RAW] udp=nonblocking");
    receiver.set_nonblocking(true)?;
    require(matches!(receiver.recv_from(&mut bytes), Err(e) if e.kind() == io::ErrorKind::WouldBlock),
            "empty nonblocking UDP did not return WouldBlock")
}

#[no_mangle]
pub extern "C" fn rust_std_probe() -> usize {
    for (name, test) in [("resolver", resolver as fn() -> io::Result<()>),
                         ("tcp", exchange), ("udp", datagrams)] {
        if name == "resolver" && !cfg!(target_os = "aros") {
            println!("[RUSTNET] case=resolver result=SKIP reason=host");
            continue;
        }
        checkpoint(match name {
            "resolver" => c"[RUSTNET_RAW] case=resolver begin",
            "tcp" => c"[RUSTNET_RAW] case=tcp begin",
            _ => c"[RUSTNET_RAW] case=udp begin",
        });
        if let Err(error) = test() {
            checkpoint(c"[RUSTNET_RAW] result=FAIL");
            if let Ok(detail) = std::ffi::CString::new(format!("[RUSTNET_RAW] case={name} error={error:?}")) {
                checkpoint(&detail);
            }
            eprintln!("[RUSTNET] case={name} result=FAIL error={error}");
            return 0;
        }
        println!("[RUSTNET] case={name} result=PASS");
    }
    checkpoint(c"[RUSTNET_RAW] result=PASS");
    let scope = if cfg!(target_os = "aros") { "ipv4-loopback" } else { "host-test-logic" };
    println!("[RUSTNET] result=PASS scope={scope}");
    3
}

#[cfg(not(target_os = "aros"))]
fn main() {
    println!("[RUSTNET] host logic check; native resolver bridge excluded");
    assert_eq!(rust_std_probe(), 3);
}
