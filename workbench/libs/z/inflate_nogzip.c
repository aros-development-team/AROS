/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: inflate for the z-nogzip.static link library
*/

/*
 * z-nogzip.static serves programs that only call compress() and
 * uncompress(). Of zlib's inflate.c these reach inflateInit(), inflate() and
 * inflateEnd(), and the reset functions called from there.
 *
 * An object of a static link library is always linked as a whole. This file
 * therefore builds inflate.c with its remaining public entry points turned
 * into unused static functions under another name, which the compiler
 * discards together with the helpers only they use. The code of the entry
 * points that are kept is that of inflate.c, unchanged.
 *
 * zutil.h is included first: it brings in zlib.h, so that the prototypes of
 * the public functions are seen before their names are redefined, and the
 * header guards keep inflate.c from including it again.
 */

#include "zutil.h"

#define NOGZIP_UNUSED(name) static __attribute__((unused)) nogzip_unused_ ## name

#define inflatePrime            NOGZIP_UNUSED(inflatePrime)
#define inflateGetDictionary    NOGZIP_UNUSED(inflateGetDictionary)
#define inflateSetDictionary    NOGZIP_UNUSED(inflateSetDictionary)
#define inflateGetHeader        NOGZIP_UNUSED(inflateGetHeader)
#define inflateSync             NOGZIP_UNUSED(inflateSync)
#define inflateSyncPoint        NOGZIP_UNUSED(inflateSyncPoint)
#define inflateCopy             NOGZIP_UNUSED(inflateCopy)
#define inflateUndermine        NOGZIP_UNUSED(inflateUndermine)
#define inflateValidate         NOGZIP_UNUSED(inflateValidate)
#define inflateMark             NOGZIP_UNUSED(inflateMark)
#define inflateCodesUsed        NOGZIP_UNUSED(inflateCodesUsed)

#include "inflate.c"
