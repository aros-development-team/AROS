# Console handler and clipboard regression tests

Run the Python regression programs with Python 3 and a host C compiler that
supports address and undefined-behavior sanitizers. They compile the relevant
production routines with controlled dependencies and failure injection.
Additional paste tests are in `rom/filesys/console_handler/tests`.

## Optional ConClip text protocol

`conclip-write-regression.py` checks validation, empty text, short writes,
chunk-pop errors and cleanup. The v1 `ConClipTextMessage` layout and READ_TEXT
operation number remain unchanged. WRITE_TEXT (0x101) borrows the caller's
buffer until reply and accepts up to `CONCLIP_WRITE_MAX` (1 MiB). It requires
`length <= capacity`, a null `sgw`, and valid data for nonempty text. Empty
writes create an empty FTXT/CHRS. ERROR does not promise rollback after I/O
begins. Unknown versions and operations remain UNSUPPORTED.

ConsoleNG may use its legacy write fallback only after UNSUPPORTED, and only
for lengths representable by SGWork.NumChars (32767 bytes). ERROR must not
trigger a second write. Callers retain message and data storage until reply.

`conclip-read-regression.py` checks multi-chunk streams through 1 MiB,
empty/missing text and injected setup, scan, read and allocation failures.
READ_TEXT concatenates all FTXT/CHRS chunks and publishes only after EOF.
Partial data is freed on error. Capacity includes spare framing bytes and is
the size callers must use when freeing the returned allocation.

## Handler lifecycle and borrowed windows

`handler-close-regression.py` checks final ACTION_END success, startup failure
and teardown ordering. Successful final close returns DOSTRUE after cleanup;
startup errors remain errors.

`completion-window-regression.py` verifies that classic consoles keep their
handler-owned window while ConsoleNG supplies its public ConUnit window as a
borrowed requester parent. Headless devices are not cast to ConUnit, and the
handler never assumes ownership of the borrowed window.

`appwindow-lifecycle-regression.py` covers library, window, port and registration
failures, duplicate registration/removal and cleanup ordering. A drop target
must be removed before CloseDevice destroys a device-owned window.

## Native validation

The Contrib ConsoleNG tests include native clipboard, query, completion and
window-lifecycle probes. Run provider-replacement fixtures only in an isolated
guest: they temporarily hide and restore the ConClip port. Install matching
protocol headers, provider and handler for those tests. Host checks alone do
not validate native rendering, a release image or physical hardware.
