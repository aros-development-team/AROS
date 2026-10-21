# AArch64 ancillary-message memory checks

This fixture checks actual libc AROS CMSG helper bodies. It does not open sockets or establish availability of any protocol named by a constant.

Apply the integrated native libc0.2.190 patch to a verified stock crate, then run:

```sh
python3 developer/debug/test/native-cmsg/host.py --libc /path/to/patched/libc-0.2.190 --output /new/output
```

The host fixture extracts the production helper module and its msghdr/cmsghdr declarations, then exercises real memory. Native `probe.rs` imports the complete actual target libc instead; compile that crate as an rlib with the retained target compiler/sysroot, pass it with `--extern libc=/path/to/rlib`, and link the probe against the matching precompiled std and C SDK. No build-std or full SDK build is needed for this probe. Require `[CMSG] result=PASS checks=24 scope=pure-memory-aarch64` and command return zero. A successful host run alone is not native qualification.

Cases include short/exact FIRSTHDR buffers, null previous header, aligned next headers at/exceeding the end boundary, maximum length and integer overflow rejection, DATA payload placement, and SPACE/LEN alignment. Caller obligations for the unsafe libc interfaces still apply; the probe does not exercise invalid dereferenced pointers. The selected AArch64 AROS headers align ancillary records to16 bytes. Helpers are restricted to this architecture until other ABI profiles are independently matched.
