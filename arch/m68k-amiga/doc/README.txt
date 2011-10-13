These are the sources to the AROS amiga-m68k AmiWest 2011 release

They are designed to used for cross-compilation from a Linux
environment.

To build the AROS amiga-m68k crosscompiler:
-------------------------------------------

* ./build-toolchain.sh </toolpath>
  - This builds a new AROS amiga-m68k toolchain, and
    installs it to </toolpath>
  - You need to have permission to modify </toolpath>

To build from the AROS sources:
-------------------------------

* Make sure that you have the libpng development headers and libraries
  installed on your build system
* Unpack the AROS sources into `AROS'
* mkdir AROS-build
* cd AROS-build
* export PATH=/toolpath:$PATH
* ../AROS/configure --target=amiga-m68k --with-optimization=-Os --serial-debug --enable-debug
* make -s bootiso

ROM images, the AROS boot disk, and the ISO image will be in
AROS-build/distfiles/

