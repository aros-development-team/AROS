#ifndef DOS_DOSHUNKS_H
#define DOS_DOSHUNKS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: File hunks.
    Lang: english
*/

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

#define EXT_SYMB      0
#define EXT_DEF       1
#define EXT_ABS       2
#define EXT_RES       3
#define EXT_REF32     129
#define EXT_ABSREF32  129
#define EXT_COMMON    130
#define EXT_ABSCOMMON 130
#define EXT_REF16     131
#define EXT_RELREF16  131
#define EXT_REF8      132
#define EXT_RELREF8   132
#define EXT_DEXT32    133
#define EXT_DEXT16    134
#define EXT_DEXT8     135
#define EXT_RELREF32  136
#define EXT_RELCOMMON 137
#define EXT_ABSREF16  138
#define EXT_ABSREF8   139

#define HUNKB_ADVISORY      29
#define HUNKF_ADVISORY (1L<<29)
#define HUNKB_CHIP          30
#define HUNKF_CHIP     (1L<<30)
#define HUNKB_FAST          31
#define HUNKF_FAST     (1L<<31)

#endif /* DOS_DOSHUNKS_H */
