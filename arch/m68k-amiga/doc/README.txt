Amiga M68K Toolchain
--------------------

Run the 'build-toolchain.sh' script to fetch,
patch, compile, and build the Binutils 2.10
and GCC 4.5.1 toolchains, and install them
to the directory of your choice:

$ ./build-toolchain.sh /some/path

If '/some/path' is not supplied, /opt/m68k-elf is
the default.

The generated toolchain will only build ELFs, but
the AROS ROM image can load both ELF and HUNK
binaries.

--
Jason S. McMullan
