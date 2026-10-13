# CD update-open regression

Build with `test-filesys-cdupdate` or `test-filesys-cdupdate-quick`.
Run `cdupdate` with a write-protected CD mounted as `CD0:` containing
`intro.dat` (at least 64 bytes). The fixture directory supplies that file;
no game image is required.

Create an ISO from the fixture directory, for example on macOS:

```sh
hdiutil makehybrid -iso -joliet -o /tmp/cdupdate.iso fixture
```

Mount the image as the guest CD and run the test. Success prints
`CDUPDATE TEST: 0 failures` and returns zero; failures return 20.

The test verifies update-open, reading and seeking, write error reporting,
directory rejection, and preservation of the original file after attempts
to write, truncate, or create a file. It refuses to run on writable media.
