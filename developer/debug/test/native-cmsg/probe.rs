// Copyright (C) 2026 John KNIPPER <code@jkn.me>.
// Distributed under the AROS Public License 1.1.
extern crate libc;
mod cases;
fn main() { let count=cases::checks(); println!("[CMSG] result=PASS checks={count} scope=pure-memory-aarch64"); }
