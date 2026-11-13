
AHI version 6
¯¯¯¯¯¯¯¯¯¯¯¯¯
This is AHI version 6.  Yeah, really.  It's been a long road but it's
finally here. No more excuses. :-)

If you have installed earlier beta versions of the AHI preference program,
MUST install the new one, go to the "advanced" page, reset the default
anti-click time and save your preferences; a data structure has changed.

Developers, take care with the new include files!  A couple of things have
changed there as well:

* In the audio mode requester structure, ahiam_UserData has been moved to
  become 32-bit aligned.  AHI still works with old programs, of course, but
  NEW programs will not work with the old device, unless you use the tag
  AHIR_ObsoleteUserData instead of AHIR_UserData.

* In the preference file structure, ahigp_AntiClickTime has been 32-bit
  aligned as well.  Since this was added in a beta version, this
  modification is not backward compatible.

I'd estimate that exactly one program is affected by these changes (the AHI
preferences program), but maybe there are a few more.  Sorry about that.


Things I'd like to do now
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 · Fix all the bugs you find.
 · Rethink everything while still maintaining full backward
   compatibility. See you in V7!


How to make your very own version of AHI
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
To configure on an Amiga (for building the AmigaOS version):

sh configure --host=m68k-amigaos


To configure on an Amiga (for building the MorphOS version):

sh configure --host=ppc-morphos


To configure on a PC running Linux (for building the AmigaOS version):

./configure --host=m68k-amigaos


Yeah, it's that easy.

You can build in separate directories, which is very useful when building
several versions from the same source tree (note:  this requires
UnixDirs3!):

mkdir build060
cd build060
sh ../ahisrc/configure --host=m68k-amigaos --with-cpu=68060


There are also a few configure options to set:

  --with-os-includedir    Path to AmigaOS include directory [/gg/os-include]
  --with-cpu              The CPU to optimize for

--with-os-includedir specifies where the AmigaOS include files are located.
If unspecified, /gg/os-include is assumed.

--with-cpu lets you specify what CPU to generate code and optimize for. Some
legal values include 68020, 68030, 68040 or 68060 for m68k and 603 or 604
for PPC. The default is either 68020 or 603, depending on the host.


The following make targets are available:

all:
	Makes all binaries, but only one version of the device.

clean:
	Removes all binaries and temporary files.

distclean:
	Like clean, but also removes files created by 'configure'.

maintainer-clean:
	Removes all machine-generated files, including the autoconf files.

bindist:
	Builds the binary distribution.  You can change the directory in
	which the binary distribution will be created, by setting the
	variable DISTDIR. The default is '/tmp/ahi'. This will build all
	possible versions of the device, including the PowerPC version.

revup:
	Increases the revision of the distribution.


When you add code, please remember a few things:

  · Avoid TAB characters, if possible.
  · The TAB size is 8.
  · The indent level is 2.
  · Compiler warnings are NOT acceptable.


Required programs/packages
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Here is a short list of required tools that I'm aware of:

From GeekGadgets:

autoconf	autoconf-2.13-bin.tgz
gcc/egcs	gcc-2.95.1-bin020.tgz
make		make-3.77-bin.tgz
unixtex		unixtex-6.1b-bin?.tgz
texinfo		texinfo-3.12-bin.tgz

Other tools from Aminet:

FD2Pragma	dev/misc/fd2pragma.lha
FlexCat		dev/misc/FlexCat.lha
PhxAss		dev/asm/PhxAss.lha
RoboDoc		dev/misc/robodoc.lha
SFDC		dev/gcc/sfdc.lha
UnixDirs3	util/boot/UnixDirs3.lha	(when using separate build directories)
