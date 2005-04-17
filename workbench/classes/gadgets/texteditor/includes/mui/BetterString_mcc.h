/*
**
** $VER: BetterString_mcc.h V11.6 (19-Dec-00)
** Copyright © 2000 Allan Odgaard. All rights reserved.
**
*/

#ifndef   BETTERSTRING_MCC_H
#define   BETTERSTRING_MCC_H

#include "amiga-align.h"

#ifndef   EXEC_TYPES_H
#include  <exec/types.h>
#endif

#define   MUIC_BetterString     "BetterString.mcc"
#define   BetterStringObject    MUI_NewObject(MUIC_BetterString

#define MUIA_BetterString_Columns       0xad001005
#define MUIA_BetterString_NoInput       0xad001007
#define MUIA_BetterString_SelectSize    0xad001001
#define MUIA_BetterString_StayActive    0xad001003
#define MUIA_BetterString_KeyUpFocus    0xad001008
#define MUIA_BetterString_KeyDownFocus  0xad001009
#define MUIM_BetterString_ClearSelected 0xad001004
#define MUIM_BetterString_FileNameStart 0xad001006
#define MUIM_BetterString_Insert        0xad001002

#define MUIV_BetterString_Insert_StartOfString  0x00000000
#define MUIV_BetterString_Insert_EndOfString    0xfffffffe
#define MUIV_BetterString_Insert_BufferPos      0xffffffff

#define MUIV_BetterString_BufferPos_End         0xffffffff

#define MUIR_BetterString_FileNameStart_Volume  0xffffffff

struct MUIP_BetterString_ClearSelected {ULONG MethodID; };
struct MUIP_BetterString_FileNameStart {ULONG MethodID; STRPTR buffer; LONG pos; };
struct MUIP_BetterString_Insert        {ULONG MethodID; STRPTR text; LONG pos; };

#include "default-align.h"

#endif /* BETTERSTRING_MCC_H */
