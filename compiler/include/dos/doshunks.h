#ifndef DOS_DOSHUNKS_H
#define DOS_DOSHUNKS_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definition of hunks, used in executable files.
    Lang: english
*/

/* Hunk types */
#define HUNK_UNIT          999
#define HUNK_NAME         1000
#define HUNK_CODE         1001
#define HUNK_DATA         1002
#define HUNK_BSS          1003
#define HUNK_RELOC32      1004
#define HUNK_ABSRELOC32   1004
#define HUNK_RELOC16      1005
#define HUNK_RELRELOC16   1005
#define HUNK_RELOC8       1006
#define HUNK_RELRELOC8    1006
#define HUNK_EXT          1007
#define HUNK_SYMBOL       1008
#define HUNK_DEBUG        1009
#define HUNK_END          1010
#define HUNK_HEADER       1011
#define HUNK_OVERLAY      1013
#define HUNK_BREAK        1014
#define HUNK_DREL32       1015
#define HUNK_DREL16       1016
#define HUNK_DREL8        1017
#define HUNK_LIB          1018
#define HUNK_INDEX        1019
#define HUNK_RELOC32SHORT 1020
#define HUNK_RELRELOC32   1021
#define HUNK_ABSRELOC16   1022

/* Extended Hunk Format types */
#define HUNK_PPC_CODE     1257
#define HUNK_RELRELOC26   1260

/* Hunk sub-types */
#define EXT_SYMB      0   /* symbol table */
#define EXT_DEF       1   /* definition for relocatable hunks */
#define EXT_ABS       2   /* definition for absolute hunks */
#define EXT_REF32     129 /* 32bit absolute reference to symbol*/
#define EXT_ABSREF32  129
#define EXT_COMMON    130 /* 32bit absolute reference to common block */
#define EXT_ABSCOMMON 130
#define EXT_REF16     131 /* 16bit relative reference to symbol */
#define EXT_RELREF16  131
#define EXT_REF8      132 /* 8bit relative reference to symbol */
#define EXT_RELREF8   132
#define EXT_DEXT32    133 /* 32bit relative data */
#define EXT_DEXT16    134 /* 16bit relative data */
#define EXT_DEXT8     135 /* 8bit relative data */
#define EXT_RELREF32  136 /* 32bit relative reference to symbol */
#define EXT_RELCOMMON 137 /* 32bit relative reference to common block */
#define EXT_ABSREF16  138
#define EXT_ABSREF8   139

/* Extended Hunk Format sub-types */
#define EXT_RELREF26  229

/* Hunk flags */
#define HUNKB_ADVISORY 29 /* Hunk is ignored, if unknown to loader. */
#define HUNKB_CHIP     30
#define HUNKB_FAST     31
#define HUNKF_ADVISORY (1L<<HUNKB_ADVISORY)
#define HUNKF_CHIP     (1L<<HUNKB_CHIP)
#define HUNKF_FAST     (1L<<HUNKB_FAST)

#endif /* DOS_DOSHUNKS_H */
