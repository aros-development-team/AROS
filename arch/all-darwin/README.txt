 This file contains various notes about Darwin-hosted port of AROS.

 1. COMPILING

 In order to compile it natively under MacOS X you need:

a) MacPorts. From it you need to install iconv, gmp, mpfr and netpbm. Three former packages are
   needed by cross-gcc.
b) AROS-targetted crosscompiler. x86-64 version (running MacOS X Snow Leopard) can be found on
   AROS Archives: http://archives.aros-exec.org/index.php?function=browse&cat=development/cross

 That's all. Execute "./configure", then "make".
 Note that current MacOS X uname still replies 'i386' about machine's architecture despite it's
actually x86-64. So by default configure will set up i386 build. There is currently no x86-64
Darwin-hosted, this is in my TODO list.
 If at some point you notice that configure started selecting x86-64 version, you may specify
target explicitly:

 ./configure --target=darwin-i386

 Native build is verified to be succesful using MacOS X v10.6 (Snow Leopard), 
+ gcc 4.2.1 (bundled with MacOS) + i386-aros-binutils v2.20.1 + i386-aros-gcc v4.4.2.

 Cross-compilation should be possible provided that you get i386-darwin-targetted crosstoolchain
up and running. I haven't verified this.

 2. RUNNING

 In order to run AROS open a command line, go to root AROS directory ("AROS"), and run
"boot\AROSBootstrap.exe". This port behaves like UNIX-hosted, it uses emul.handler, which
makes your current directory to be root of your SYS:.
 You can specify some options on the command line for bootstrap and AROS. Enter
"boot\AROSBootstrap.exe -h" to learn about bootstrap's options. Also you may check
AROSBootstrap.conf file for some settings. There are also some other useful options understood
by AROS itself:

 vblank=xx - set emulated vertical blank frequency to xx Hz. Default is 50.
 eclock=xx - set master system's timer to xx Hz. Default value is 50. Old Linux-hosted version
             had builtin default equivalent to eclock=100. This may impact time measurement quality.

 09.11.2010, Pavel Fedin <pavel_fedin@mail.ru>