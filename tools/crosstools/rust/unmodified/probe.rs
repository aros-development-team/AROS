// Copyright (C) 2026 John KNIPPER <code@jkn.me>
// Licensed under the AROS Public License (APL) Version 1.1

use std::{fs, net::TcpStream, process::Command, thread};

// This retains representative std paths in the archive/link probe. It is not
// a runtime acceptance suite: runtime networking is unqualified and process
// operations remain unsupported.
#[no_mangle]
pub extern "C" fn rust_std_probe() -> usize {
    let worker = thread::spawn(|| fs::read_dir("RAM:").map(|entries| entries.count()).unwrap_or(0));
    if let Err(error) = TcpStream::connect("127.0.0.1:1") {
        eprintln!("network probe: {error}");
    }
    if let Err(error) = Command::new("C:Echo").arg("probe").output() {
        eprintln!("process probe: {error}");
    }
    worker.join().unwrap_or(0)
}
