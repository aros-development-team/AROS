 This is CEBoot - an incomplete (at the moment), experimental Windows CE-hosted softkicker for native AROS.

 This project is inspired by HaRET Linux bootloader (http://htc-linux.org/wiki/index.php?title=HaRET).
However it's neither HaRET port nor based on HaRET's code. This is a clean implementation of AROS bootloader,
using just some techniques learned from HaRET source code. It is more based on hosted AROSBootstrap, however
significantly reengineered to meet needs of native kickstart.

 CEBoot is written with portability accross various CPUs in mind, however is developed on ARM device (HTC Touch2
smartphone). The only CPU-dependent code is going to be MMU trampoline (not written yet). It is currently compiled
with arm-mingw32ce-gcc, to which $KERNEL_CC is set when AROS is configured with --target=mingw32-arm (originally
standing for ARM WindowsCE-hosted build). When native ARM AROS involves, the build system will be improved to allow
building this bootstrap alongside any native port. Since CEBoot is designed to be portable, it can be improved in
order to allow booting up AROS on i386 devices originally running Windows CE. The bootloader will need to supply
a proper Multiboot memory map to the kickstart in order to do this.

                                                                23.12.2011 Pavel Fedin <pavel_fedin@mail.ru>