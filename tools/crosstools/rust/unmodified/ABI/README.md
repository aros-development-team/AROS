# Native network ABI qualification

`network-abi.py` compiles constant records from the installed AROS SDK and the
actual `libc` rlib produced by `../build.py`. It extracts `.aros_abi` from both
AArch64 objects with `llvm-objcopy`; no guest execution is required.

```sh
python3 network-abi.py \
  --sdk /absolute/path/to/Developer \
  --crosstools /absolute/path/to/aros-crosstools \
  --std-build /absolute/path/to/qualified-std-build \
  --output /absolute/path/to/new-artifact-directory
```

The std build must contain `result.json` with `std_compile: PASS`, its `rustc`
wrapper, the JSON target, and exactly one `core` and `libc` rlib in
`sysroot/lib/rustlib/aarch64-unknown-aros/lib`. The output must be a new directory
outside all three input trees. The tool records commands, diagnostics, generated
C/Rust sources, objects, extracted bytes, and a JSON comparison. It fingerprints
the consumed SDK headers and std/tool inputs and verifies them after compilation.

The records cover sizes, alignment, member offsets and member sizes for socket
addresses, resolver records, scatter/gather messages, ancillary message headers,
linger and timeout records, plus socket options, protocol identifiers and ioctl
values. `MSG_NOSIGNAL` is an optional capability: absence in both SDK and Rust is
reported as unsupported. A Rust value with no native counterpart fails.

Exit status 1 means a direct ABI mismatch; 2 means input validation or compilation
failed. Exit status 0 means `DIRECT_ABI_MATCH_BRIDGE_UNQUALIFIED`, not runtime
acceptance. `addrinfo` belongs to the explicit resolver conversion boundary and
its mismatches are reported separately. The comparison never qualifies that
conversion or any networking behavior on target hardware.

Pass `--application-libc-rlib /path/to/liblibc-<hash>.rlib` to compare the actual
application store crate (for example libc 0.2.190) instead of std's internal
libc. The wrapper, core and target identity still come from `--std-build`; the
selected application rlib and its dependency path are fingerprinted separately.

`libc-exports.py --std-build BUILD --application-libc-rlib RLIB
--crosstools TOOLS --output OUTPUT` extracts all ELF members of both libc
archives and links them explicitly into one relocatable object. This forces
collisions that ordinary lazy archive extraction could hide. It rejects Rust
crate definitions of the unavailable C interfaces and, with `--binary FILE`,
requires exactly one native provider for the probed application calls.
