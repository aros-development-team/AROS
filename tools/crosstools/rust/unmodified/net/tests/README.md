# Resolver conversion host tests

Run from any working directory, giving a fresh artifact path:

```sh
python3 tools/crosstools/rust/unmodified/net/tests/run.py --output /tmp/aros-resolver-host
```

The runner compiles the actual `../resolver.c` as a separate translation unit,
with only its allocator names redirected for deterministic fault injection.
The fake native header models AROS LP64 `addrinfo` (four 64-bit `long` fields,
64-bit `size_t`, 64-byte record), byte-sized socket length/family, IPv4/IPv6
records and AROS `AF_INET6=28`. It does not include host socket/netdb headers.
The fixture is not a substitute for the real SDK object ABI comparison.

The fixture verifies deep copies after native storage is poisoned, exact hints
conversion, rejection of ancillary hints, all five allocation failures including
rollback after an earlier result, integer overflows, invalid address pointer,
length, family and socket length, provider errors with and without results,
missing provider, empty results, errno retention, native free/close pairing,
immutable error messages, and disposal on another thread after provider close.
Eight worker threads rendezvous inside their first lookup with all eight
per-call bases live, then perform 400 lookups total.

ASan and UBSan run with fatal diagnostics. A per-thread allocation ledger checks
that conversion rollback and result disposal release every successful bridge
allocation. Darwin's ASan runtime does not support LeakSanitizer, so the runner
explicitly disables that feature there; other hosts enable it.

The artifact directory retains compiler version, command logs, source snapshots,
SHA-256 identities and a strict `result.json`. A source change during the run
fails the result. This qualifies host conversion logic, not native resolver calls,
AROS library lifecycle implementation, DNS service availability or hardware.
