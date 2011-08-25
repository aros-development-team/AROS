 1. INTRODUCTION.

 This is Google Android-hosted port of AROS. Android is a subset of Linux system, so many functionality
of this port is the same as of a generic Linux-hosted port.
 At the moment only ARM architecture is supported. i386 version can be created with minor adjustments.

 A minimum required Android version is 2.2 (API level 8).

 2. COMPILING.

 In order to compile Android-hosted AROS you need:

 - A usual prerequisites for building AROS (GNU/UNIX-compatible environment). If you are running Windows, MinGW
   (MSYS) is the recommended one. Cygwin is much slower, and it will unlikely work with Android NDK because gcc
   versions supplied with modern Android NDK are MinGW-hosted, not Cygwin. Don't be confused by the fact that
   NDK's own build system is claimed to require Cygwin. AROS doesn't use this build system, it uses only gcc
   compiler, includes and link libraries from the NDK. NDK build system, when used, translates paths from UNIX
   to Windows style for the gcc; AROS build system won't do the same under Cygwin. If running Cygwin, it assumes
   all tools are Cygwin-hosted. It's possible to build Android-targetted gcc for Cygwin, but this was never tested.
   At the moment Cygwin is generally deprecated for building AROS, and Cygwin-related portions in the configure
   script are poorly maintained.

   One note: MinGW project does not offer netpbm package on their web site. It can be found in another project,
   GNUWin32. GNUWin32 ports perfectly cooperate with MinGW ones.

 - ARM AROS-targeted crosscompiler. At the moment only MacOS X/x86-64 version can be found on AROS Archives
   as binary distribution. You have to build own AROS toolchain if running another system. The process is
   straightforward and explained in README files for gcc and collect-aros. Please feel free to release your
   built toolchains on AROS Archives in order to assist other developers.

   Note that Android-targetted gcc can't be used for building AROS binaries even with wrapper scripts.
   Android ABI is not 100% compatible to AROS one.

 - Android SDK (Java one). The new build was succesfully tested with SDK r10 on MacOS X v10.6.8 (x86-64).
   During early development stages SDK v1.6r1 was used under Windows. In fact any SDK version should work.

 - Android NDK (Native Development Kit). Versions statring from r5 are known to be supported. Earlier versions
   are not expected to work, because directory tree structure was changed.
   Android NDK has an option to generate a "standalone" version of Android toolchain. For AROS this is options.
   AROS build system is expected to support such toolchains, but this was not tested.

 - Apache Ant or Eclipse. These are optional, however without them you won't be able to build Java application
   containing the bootstrap. For automated builds (e. g. nightly build) Ant is mandatory.

 Configure script needs the following additional parameters to set up Android-hosted build:

 --target=linux-<cpu>            - Android is a subset of Linux architecture

 --enable-target-variant=android - This actually tells to set up Android-hosted build

 --with-sdk=<path>		 - Specifies location of installed Android SDK. This parameter currently can't be
				   omitted at all. There is some default for it, however it is unlikely to fit
				   your machine, since the SDK does not suggest any specific default location by
				   itself.

 --with-ndk=<path>		 - Specifies location of installed Android NDK. If you omit this, AROS build system
				   will expect to use standalone toolchain installed in your $PATH.

 --with-gcc-version=<number>     - If you are not using a standalone Android toolchain generated from the NDK,
				   you must specify it. Otherwise AROS won't be able to locate a gcc inside it.
				   NDKs r5 and r6 are both supplied with version 4.4.3. If this ever changes,
				   you can easily check which gcc version is contained in your NDK. Just look
				   into "toolchains" subdirectory.

 --with-sdk-version=<number>     - The value of this parameter is Android API level number (not OS version).
				   It specifies platform version to use within the SDK. SDK r10 supports
				   platforms 8 and 11, and value of this parameter defaults to 8 (minimum
				   requirement for AROS). You'll need to specify this parameter if API level 8
				   is ever dropped from the SDK.
				   You can easily look up what platform versions are supported by your SDK by
				   examining its 'platforms' subdirectory.

 --with-ndk-version=<number>	 - Similar to --with-sdk-version, but specifies API level for the NDK. This is
				   different from SDK because: a) SDK doesn't include all levels included in the
				   NDK; and b) AROS display driver needs some includes from API level 9 (despite
				   AROS runs on level 8). This parameters defaults to 9 and will be needed if
				   older API levels are dropped from the NDK at some point.

 Android documentation says that APKs must be signed in order for the system to allow installing them.
AROS build system is set up to sign the built application with "AROS" key located in $HOME/.android/AROS.keystore.
If you are setting up the build for the first time, you need to create this key. In order to do this you need
to use "keytool" utility supplied with the JDK:

 keytool -genkey -keystore ~/.android/AROS.keystore -alias AROS -validity 366000

 Unfortunately, a certificate must have validity period. In this example we set it to more than 1000 years
(for simplicity we count every year for 366 days). The longer the time, the better. AROS is not going to die! :)

 After this command keytool will ask for your personal data. It's recommented to fill them in correctly, this can
help to identify origins of the build. When the keytool asks you for key password, use 'aros.org'. This password
is specified in build.properties file of the Java project, this allows ant to operate fully automatically and
not to ask you for the password every time. This is especially useful for automated (e. g. nightly) builds.

 Remember that AROS is an open project! It doesn't honor any conspiracy. Everyone is allowed to make builds and
publish them. This key exists only because Android mandates it. It's neither meant to be used for some private
authentication, nor serves for enforcing some limitations on the end user. These are reasons why key password
is stored open in the SVN tree. It's possible to adjust the build system to use arbitrary system-specific key
and password, but this is not done for simplicity reasons.

 3. INSTALLING.

 In order to install AROS on your Android device, you need:

 - Copy 'AROS' directory with all its contents to the root of your SD card (or whatever else external storage).
 - Instal AROSBootstrap.apk like any other application.

 4. RUNNING.

 There's nothing particular about this. Run 'AROS' application and enjoy.
 Note that AROS was initially developed as a desktop OS. Android-hosted port is the first completed port to run on
mobile devices. Consequently, AROS lacks many features that could make it a mobile system. First of all, AROS lacks
a virtual keyboard. Second, user interface is not optimized to be operated by fingers and/or keypad. If your device
has a keyboard, AROS will support it, however this was not tested yet. 'Menu' key found on most of Android devices
emulates right mouse button and 'Back' button emulates LAmiga+M sequence (screen switch).

 For more comfortable use you can do the following:

 - Turn on 'Sticky menus' option in IControl preferences.
 - Enable popup system menus.
 - Set larger fonts for user interface, to increaze size of controls.

 Note that trackball found on NexusOne phone is of a very poor quality. It is supported by AROS as a mouse, however
the cursor speed will be very slow, even if you turn on the acceleration. This is a property of the trackball itself
and it can't be fixed by software. Android OS uses it to emulate keypad, not as a mouse. Perhaps AROS should do this
too, this is subject to further development.

 There is intentionally no support kludges that would make use of Android's own virtual keyboard. AROS is not an
emulator, it's an operating system by itself, and it should contain own support for such things, independently
on what it's running on. Remember that one day AROS will run natively on these devices. The same reason prevents
AROS from supporting autorotating screen. AROS user interface libraries currently has no concept of screen rotation,
and it's unclear how it can be implemented (however it's not impossible).

 An intensive development is needed to remove these limitations.

 5. DEBUGGING.

 For Java part AROS build system creates a complete Eclipse project. It's located in
$(TOP)/bin/linux-$(CPU)-android/gen/arch/all-android/bootstrap/app. You can import it into Eclipse workspace and
work with it as usually. On UNIX systems the project will contain symlinks to original directories, so files
modified in Eclipse will stay in their original place. Unfortunately on Windows this is not possible, and directories
will be copied instead (unless MinGW implements support for post-XP symlinks). So, if you modify something there,
you'll have to copy your changes back to SVN tree. Theoretically it's possible to work around it by modifying Java
build paths inside the project, i haven't tried it.

 Debug output from AROS itself is copied to Android debug buffer. You can read it as usually, using 'logcat' command
in Android shell. Also you can use 'logfile' option in AROSBootstrap.conf in order to save the output to a file, the
same as in any other hosted port.

 Androd NDK documentation describes a way to attach gdb to a process on the device. This was not tested with AROS,
however is expected to work.

 AROS is expected to run inside Android emulator, since it's full ARM machine emulator, not a hosted environment
like for example iOS or Symbian simulators. I haven't tried it because of: a) complicated process of SD card image
creation; and b) overall slowness of the emulator even on very fast computers. It was much faster and easier to use
the real hardware for the development.

 6. HACKING.

 Android is a very hostile environment to a native code. All Android applications are running inside a Java VM.
It uses UNIX signals for own purposes, conflicting with AROS multitasking. This makes it impossible to run AROS
directly in application's context. It was proved during early port development. There's no way to shut down
Java environment.

 In order to work around this limitation, AROS runs in its own process asynchronously. Comminication with Android
OS is implemented using client-server architecture. Before running AROS via fork(), bootstrap creates two pipes
(see Kick() method implementation). These pipes are used as communication channels to and from display server,
written in Java. AROS display driver sends action commands to the server, and gets input events from it. This is
the only way for AROS to communicate with Android APIs other than Linux kernel and C libraries on top of it.
 It's unknown whether it's possible to call Android GL API from within AROS under this conditions.

 Because of this, please, keep communication with Java at a minimum. In order to prevent from de-synchronization
all commands are processed by display server in the same order as in which they arrived. In order to satisty this,
Java display server thread forwards all commands to main user interface thread, where they are processed and
replied. Such an architecture very negatively impacts on the overall AROS performance, especially when a command
needs a reply, and it's a good practice to stay away from it. It's impossible to remove a separate display server
thread because Java API lacks asynchronous I/O facilities which could be used together with Android user interface
processing.
 Fortunately Android's lower layers (kernel and C libraries) do not suffer from any limitations, and at least basic
system services (file access, time, signals, etc) can be freely used by AROS. If someone wants to try his hands on,
for example, accelerometer support for AROS, the primary way to go would be direct interfacing with the device using
/dev directory.

 If you work with code using communication pipes, remember the general rule: pipe error on any side should cause
immediate process exit. The pipe can be broken if one of processses exit. In this case, if the user interface exits,
and AROS fails to notice this, you'll end up in blindly running AROS process without any control on it. It's impossible
to even use kill command on non-rooted device, so the only way to get rid of such process (draining the battery and
system resources) is to completely reboot the device. Please do everything possible to avoid this!

 An alternate way to run hosted AROS on Android device would be total shutdown of Java environment and taking over
the device. AROS would need a Linux framebuffer display and raw hardware input drivers. There is very old source code
of these drivers which is slowly being reworked. The only drawback of such approach is that cellular phone will stop
being serviced. AROS would need to take care of it itself then. Additionally, it would require a rooted device.

									Pavel Fedin <pavel_fedin@mail.ru>
