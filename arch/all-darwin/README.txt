 This file contains various notes about Darwin-hosted port of AROS.

 1. COMPILING

 In order to compile it natively under MacOS X you need:

a) MacPorts. From it you need to install iconv, gmp, mpfr and netpbm.
   Three former packages are needed by cross-gcc.
b) AROS-targetted crosscompiler. x86-64 version (running MacOS X Snow
   Leopard) can be found on AROS Archives:
 http://archives.aros-exec.org/index.php?function=browse&cat=development/cross

 That's all. Execute "./configure", then "make".
 Note that current MacOS X uname still replies 'i386' about machine's 
architecture despite it's actually x86-64. So by default configure will 
set up i386 build. There is currently no x86-64 Darwin-hosted, this is 
in my TODO list.
 If at some point you notice that configure started selecting x86-64 
version, you may specify target explicitly.
 Supported targets are:

 darwin-arm 
 darwin-i386
 darwin-ppc
 darwin-x86_64

 If you're cross-compiling (your host CPU differs from target CPU, for example
you're building PPC AROS on i386 Mac), you'll likely have to specify gcc version
explicitly. This is because Apple provides gcc binaries only with version suffix
(for example ppc-apple-darwin10-gcc-4.2.1 instead of just ppc-apple-darwin10-gcc).
This can be done using --with-gcc-version=x.y argument for configure. For example:

 ./configure --target=darwin-ppc --with-gcc-version=4.2.1
 
 Currently only PowerPC and i386 versions are complete. x86_64 version is a work
in progress. ARM version makes sense only for iOS variant (using
--enable-target-variant=ios switch). iOS port is also a work in progress and is
described separately.

 Building on Mac is verified to be succesful using MacOS X v10.6 (Snow 
Leopard), + gcc 4.2.1 (bundled with MacOS) + aros-binutils v2.20.1 
+ aros-gcc v4.4.2. Cross-compilation on non-Mac should be possible provided that
you get Darwin-targetted crosstoolchain up and running. I haven't tried this.

 2. RUNNING

 In order to run AROS open a command line, go to root AROS directory 
("AROS"), and run "boot/AROSBootstrap.exe". This port behaves like 
UNIX-hosted, it uses emul.handler, which makes your current directory 
to be root of your SYS:.
 You can specify some options on the command line for bootstrap and 
AROS. Enter "boot/AROSBootstrap.exe -h" to learn about bootstrap's 
options. Also you may check AROSBootstrap.conf file for some settings. 
There are also some other useful options understood by AROS itself:

 vblank=xx - set emulated vertical blank frequency to xx Hz. Default is 50.
 eclock=xx - set master system's timer to xx Hz. Default value is 50. Old
             Linux-hosted version had builtin default equivalent to
             eclock=100. This may impact time measurement quality.

 If you like some particular settings (like eclock=100) you may enter this
in AROSBootstrap.conf file after "arguments" keyword. What you specify there
will be always appended to the command line you give to the bootstrap in the
shell.

 25.11.2010, Pavel Fedin <pavel_fedin@mail.ru>
