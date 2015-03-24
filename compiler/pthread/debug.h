/*
  Copyright (C) 2015 Szilard Biro

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __DLL_DEBUG_H
#define __DLL_DEBUG_H

#ifdef __AROS__
    #include <aros/debug.h>
#else
    #ifdef DEBUG
        #define D(x) x
        #if DEBUG > 1
            #define DB2(x) x
        #else
            #define DB2(x)
        #endif
    #else
        #define D(x)
        #define DB2(x)
    #endif
    #ifdef __amigaos4__
        #include <proto/exec.h>
        #define bug DebugPrintF
    #else
        #include <clib/debug_protos.h>
        #define bug(fmt, ...) kprintf((CONST_STRPTR)fmt, __VA_ARGS__)
    #endif
#endif

#endif
