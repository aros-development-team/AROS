 1. Building ARM cross toolchain.

At the moment ARM AROS is compiled using arm-linux-gnueabi toolchain.
The following command is needed to configure binutils correctly:

./configure --target=arm-linux-gnueabi

Building GCC is a little bit more complicated. Currently we can't build the complete compiler suite because
we don't have includes and link libraries for AROS. So we have to live with barebone compiler:

./configure --target=arm-linux-gnueabi --with-fpu=neon --with-float=softfp --enable-languages=c --disable-threads --disable-shared --enable-sjlj-exceptions
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

 2. MacOS X specifics.
 
The following options need to be added to configure arguments:

 binutils: --disable-werror
 gcc     : --with-libiconv-prefix=/opt/local --with-gmp=/opt/local --with-mpfr=/opt/local

This assumes that you installed iconv, gmp and mpfr libraries from macports (this is what i did).
This was tested with binutils v2.20.1 and gcc v4.4.2.

 3. Linux specifics.

Linux kernel does not provide any standarized view of VFP context on signal frame. The ARM linux-hosted
port assumes, that the VFP frame is stored in uc_regspace[] area. This is the case on nearly all linux kernel 
compiled with VFP/NEON support. If this is not the case, or of linux misses the VFP frame in sigcontext, AROS 
will probably fail.

									Pavel Fedin <pavel_fedin@mail.ru>
