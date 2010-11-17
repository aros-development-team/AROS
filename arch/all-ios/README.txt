 1. INTRODUCTION.

 This is Apple iOS-hosted port of AROS. Currently it's incomplete and under development.
 i386 architecture support is complete and the port succesfully runs in iPhone Simulator.
Support for ARM architecture is still in progress. ARM iOS puts some limitations on application's behavior so
running AROS under iOS is slightly more complex task than running it on ARM Linux. Currently ARM versions boots
up to the point where it runs contents of DEVS:Monitors directory. The first loaded binary crashes because of
iOS' no-execution protection.
 Note that currently there's no UIKit display driver, so AROS currently displays "Unknown type of system screen"
guru. However it boots up into Workbench.

 2. COMPILING.

 In order to compile iOS-hosted AROS you currently need Apple XCode with iOS SDK. This assumes you
are running on Mac. Cross-compiling of the port seems to be possible using source code of Darwin tools,
but i haven't tried it yet.
 When configuring you need to supply --enable-target-variant=ios argument and select one of supported targets:
darwin-i386 for Simulator version and darwin-arm for real hardware.
 The following iOS-specific options are added to configure for supporting this build:
 --with-xcode       - specifies where XCode is located on your machine. Default is /Developer (this is where
                      XCode v3.2.4 installed itself by default on my machine)
 --with-ios-version - specifies minimum supported iOS version, defaults to 3.0. Actually this value is supplied
                      to gcc via -miphoneos-version-min and nothing else.
 --with-ios-sdk     - specifies iOS SDK version to use. Defaults to 4.1 (the latest SDK available with my XCode).
                      Actually this value sets the SDK's root directory (used for locating headers and libraries) in the form of:
                      $aros_xcode_path/Platforms/$aros_ios_platform.platform/Developer/SDKs/$aros_ios_platform$aros_ios_sdk.sdk
                      where:
                        $aros_xcode_path   - what is supplied by --with-xcode
                        $aros_ios_platform - "iPhoneSimulator" for ios-i386 target and "iPhoneOS" for ios-arm target
                        $aros_ios_sdk      - what is supplied by --with-ios-sdk

 3. INSTALLING.

 The bootstrap expects to locate AROS root directory inside Documents directory of its sandbox (i.e. Documents/AROS). The root does
not have to contain the bootstrap itself. The bootstrap is built as standard application bundle and is located one level up relating to
AROS directory. iTunes is known not to accept unsigned applications, so you'll have to use any third-party tools. Personally i use
Installous program (http://hackulo.us/forums/). You can put an IPA into its download directory (/var/mobile/Documents/Installous/Downloads),
then run the program, go to "Downloads" section, and you'll see AROSBootstrap there and be able to install it.
 For transfering files i use iPhone Explorer (http://www.macroplant.com/iphoneexplorer/). Unfortunately you can't install and/or
update bootstrap bundle using it, because during copying files lose their 'executable' permission bit.
 If you have Mac and XCode, you can also use XCode's organizer for installing the bootstrap application.

 4. RUNNING.

 Simulator version runs without problems.
 Device version is a little bit more problematic. One of iOS limitations is that it disallows to set protection for allocated memory to
both writable and executable at once. This means that AROS can't run any binary from disk. This is not a stopper and can be bypassed.
Currently a protected memory management system for AROS is being developed, which will allow ARM iOS version to run.
 AROS requires a jailbreak to run. I tried to create a signed version of the bootstrap and ran it on un-jailbroken iOS v3.1. As soon
as bootstrap launches the loaded kickstart it gets SIGKILL from the system. Apple does not allow applications to load and run any code.
 A RedsnOw jailbreak is known to be compatible with AROS, it is used on iPhone model 2G which i use for testing AROS.
 Debug log from AROS is directed to stderr. The easiest way to read it is using XCode's organizer window. Connect your device and go to
'console' tab. You'll see a log there. Unfortunately it's not transferred instantly, so you may have to wait a while until the last line
pops up. Re-running crashed AROS helps, the log gets flushed when new large data portion arrives.
 Attaching remote gdb to a device is a big problem. First, you need to sign your executable with valid developer certificate and
debugging entitlements. Second, you'll have to compose XCode project for the bootstrap, listing all its source files. XCode integrates
very poorly with external build systems, so personally i abandoned this idea after some tests. Attaching remote gdb to an iPhone without
XCode seems to be impossible. Communication is performed through a UNIX socket which exists only while XCode debugging session is
running.
 To make life simpler, something needs to be done in the bootstrap, like redirecting the log to a text file. Also perhaps some
third-party tools exist, which i don't know about.
 With Simulator everything is way simpler. You just open MacOS' system console and read the output.

									Pavel Fedin <pavel_fedin@mail.ru>
