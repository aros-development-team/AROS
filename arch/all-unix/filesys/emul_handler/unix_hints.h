/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Some really black magic. Adjusts compilation for various UNIX weirdness.
 * This stuff is placed in a separate file because it needs to be included
 * before any UNIX headers.
 */

#ifdef HOST_OS_ios

#ifdef __arm__
/*
 * Under ARM iOS quadwords are long-aligned, however in AROS (according to AAPCS)
 * they are quad-aligned. This macro turns on some tricks which bypass this problem
 */
#define HOST_LONG_ALIGNED
#endif
#ifdef __i386__
/*
 * Under i386 we pick up MacOS' libSystem.dylib instead of Simulator's libSystem.dylib,
 * so we have to use special versions of certain functions. We can't simply #define _DARWIN_NO_64_BIT_INODE
 * because iOS SDK forbids this (in iOS inode_t is always 64-bit wide)
 */
#define INODE64_SUFFIX "$INODE64"
#endif

#else

/*
 * Use 32-bit inode_t on Darwin. Otherwise we are expected to use "stat$INODE64"
 * instead of "stat" function which is available only on MacOS 10.6.
 */
#define _DARWIN_NO_64_BIT_INODE
#endif

#ifdef HOST_OS_darwin

#ifdef __i386__
/*
 * Under i386 there are both POSIX-conformant and non-POSIX-conformant
 * variants of certain functions, the former suffixed by $UNIX2003. Bugs
 * occur if we use non-conformant versions of some functions (e.g. rewinddir()
 * doesn't rewind), so we always use the conformant functions where there is a
 * choice
 */
#define UNIX2003_SUFFIX "$UNIX2003"
#endif
#endif

#ifndef INODE64_SUFFIX
#define INODE64_SUFFIX
#endif

#ifndef UNIX2003_SUFFIX
#define UNIX2003_SUFFIX
#endif
