 1. INTRODUCTION.

 This is Apple iOS-hosted port of AROS.
 i386 architecture support is complete and the port succesfully runs in iPhone Simulator.
The bootstrap is also compiled and tested on ARM CPU on a real iPhone. It works. However ARM
AROS kernel is still in progress.
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
 --with-ios-version - specifies minimum supported iOS version, defaults to 2.0. Actually this value is supplied
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
AROS directory. You have to install it as a usual application, on jailbroken iPhone the easiest way to do this is using iPhone Explorer
(http://www.macroplant.com/iphoneexplorer/).

 4. RUNNING.

 Simulator version runs without problems. Device version is of course not signed, it runs only on jailbroken machines. There is
some support for creating signed version, but because of proprietary nature of the thing i won't describe it here. In fact it's very
easy to discover how to create a signed bundle, just read the mmakefile.src and accompanying shell scripts. Of course you need a
developer certificate and provisioning profile for this. I use my company's test profile and it works. However i would not expect
that Apple would ever permit to release such an application in their AppStore, so code signing is a purely experimental feature,
written as part of my company's research project.
 Remember that Apple would never approve AROS for their AppStore! AROS violates at least 25% of their rules by its nature. So
do not even try it. And it's better not to distribute signed versions. If you do so, do not complain about being banned from
AppStore and/or Apple developer site. You have been warned!

									Pavel Fedin <pavel_fedin@mail.ru>
