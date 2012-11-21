/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef LOADSEG_LOADSEG_H
#define LOADSEG_LOADSEG_H

#include <aros/libcall.h>
#include <dos/elf.h>

/* This is called after ELF file is loaded. You may grab the debug info from it. */
void register_elf(BPTR file, BPTR hunks, struct elfheader *eh, struct sheader *sh, struct DosLibrary *DOSBase);

AROS_LD4(BPTR, InternalLoadSeg,
         AROS_LDA(BPTR       , fh           , D0),
         AROS_LDA(BPTR       , table        , A0),
         AROS_LDA(LONG_FUNC *, funcarray    , A1),
         AROS_LDA(LONG *     , stack        , A2),
         struct DosLibrary *, DOSBase, 126, Dos);

/* 
 * Use LoadSegment() to call the linklib's loader.
 * There's no counterpart since the loaded seglist can be freed by
 * conventional IntrernalUnloadSeg(), even on old AmigaOS.
 */
#define LoadSegment(fh, table, funcarray, stack)                        \
        AROS_CALL4(BPTR, AROS_SLIB_ENTRY(InternalLoadSeg, Dos, 126),    \
                   AROS_LCA(BPTR       , fh           , D0),            \
                   AROS_LDA(BPTR       , table        , A0),            \
                   AROS_LDA(LONG_FUNC *, funcarray    , A1),            \
                   AROS_LDA(LONG *     , stack        , A2),            \
                   struct DosLibrary *, DOSBase)

#endif /* LOADSEG_LOADSEG_H */
