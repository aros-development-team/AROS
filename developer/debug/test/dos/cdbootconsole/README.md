# CD boot console regression

Build `test-dos-cdbootconsole` or `test-dos-cdbootconsole-quick`.
The Amiga DOS test group selects this test through its architecture rules.

Run the executable directly from a CD image's `S:Startup-Sequence`. Running
through an emulator's host `--run` option tests a different boot path.
For example, build a fixture using `mkisofs`:

```sh
mkdir -p /tmp/cdbootconsole-fixture/s
cp /path/to/built/cdbootconsole /tmp/cdbootconsole-fixture/
printf 'cdbootconsole\n' > /tmp/cdbootconsole-fixture/s/startup-sequence
mkisofs -quiet -R -J -iso-level 2 -V DOSBOOT -sysid CDTV \
    -o /tmp/cdbootconsole.iso /tmp/cdbootconsole-fixture
```

Boot that image with AROS in a stock CD32 configuration. The test checks the
inherited NIL input and output handles, checks that no console process is
allocated before a query, sends a legacy DOS information packet
directly to the process console port, checks its reply, and verifies that
no console process or window is allocated by the query. Serial output reports
`CD BOOT CONSOLE TEST: 0 failures` on success; failures return 20.
Bound emulator runs to detect a missing packet reply as well as assertion failures.

The query must also leave both free Chip RAM and its largest contiguous block
unchanged. The native console state uses the existing boot stack; the Gunship
headquarters/pilot audiovisual scenarios additionally check the loading deadline
that regressed when a separate heap allocation fragmented the free block and
forced graphics allocations to reclaim the CD filesystem's caches.

The fixture also starts a synchronous copy of itself with no explicit error
stream. The child must inherit the console identity while its default streams
remain NIL. This detects accidental handler-backed error streams and their
buffer allocations during otherwise quiet CD boot.
