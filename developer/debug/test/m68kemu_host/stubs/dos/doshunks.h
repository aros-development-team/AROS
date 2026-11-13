#pragma once
#include <exec/types.h>
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
#define HUNK_PPC_CODE     1257
#define HUNK_RELRELOC26   1260
#define HUNKRELOC_ABS   0   /* longword = target base + original */
#define HUNKRELOC_REL   1   /* longword = target base + original - address of the longword */
#define HUNKRELOC_BASE  2   /* longword = target base + original - base of the hunk holding it */

struct HunkRelocGroup
{
    ULONG  hrg_Hunk;        /* hunk holding the relocated longwords */
    ULONG  hrg_Target;      /* hunk whose base was added */
    ULONG  hrg_Kind;        /* HUNKRELOC_* */
    ULONG  hrg_Count;
    ULONG *hrg_Offsets;     /* byte offsets of the longwords in hrg_Hunk */
};
#define HUNKRELOC_ABS   0   /* longword = target base + original */
#define HUNKRELOC_REL   1   /* longword = target base + original - address of the longword */
#define HUNKRELOC_BASE  2   /* longword = target base + original - base of the hunk holding it */

struct HunkRelocTable
{
    ULONG                  hrt_Groups;  /* ~0: the relocations could not be kept */
    struct HunkRelocGroup *hrt_Group;
};
