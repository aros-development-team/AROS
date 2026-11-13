#ifndef DOS_FHFLAGS_H
#define DOS_FHFLAGS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: FileHandle fh_Flags definitions, shared between dos.library
          and dos64.library. The flags are AROS specific and therefore
          PRIVATE - they describe dos.library's buffering state and
          must not be interpreted by applications.
*/

#define FHF_WRITE    0x80000000
#define FHF_BUF      0x00000001
#define FHF_APPEND   0x00000002
#define FHF_LINEBUF  0x00000004
#define FHF_NOBUF    0x00000008
#define FHF_OWNBUF   0x00000010
#define FHF_FLUSHING 0x00000020

#endif /* DOS_FHFLAGS_H */
