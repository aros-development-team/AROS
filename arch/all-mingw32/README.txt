 This file contains various notes about Windows-hosted port of AROS.

 1. COMPILING

 This port currently can only be compiled under Windows OS. In order to do this
you need:

a) Working Cygwin environment (Mingw's MSYS is expected to work too but it's not tested).
b) Netpbm package of course.
c) AROS-targetted crosscompiler. Cygwin version of it can be found on AROS Archives:
   http://archives.aros-exec.org/index.php?function=browse&cat=development/cross
   I compile it using gcc version 3.3.1, however i beleive more recent versions will
   work too.
d) Mingw32 libraries package for Cygwin.

 That's all. Execute "./configure --target=mingw32-i386", than make.

 2. RUNNING

 In order to run AROS open a command line, go to root AROS directory ("AROS"), and run
"boot\AROSBootstrap.exe". This port behaves like UNIX-hosted, it uses emul.handler, which
makes your current directory to be root of your SYS:. Default kernel file expected by the
bootstrap is "boot\kernel". You can explicitly specify another kernel to boot at bootstrap's
command line.
 Currently bootstrap allocates a fixed 100MB chunk of memory for use by AROS. In future you'll
be able to change this, i'm planning also managed memory support (like on Linux).

 26.02.2009, Pavel Fedin <sonic.amiga@gmail.com>