/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef __AROS__

#include <exec/types.h>
#define STM_SEARCH 18
#define STM_SEARCH_NEXT 19
#define STM_SEARCH_PREV 20
#define PGA_NotifyBehaviour PGA_Dummy+30
#define PG_BEHAVIOUR_NICE 1
#define PDTA_DitherQuality DTA_Dummy+222
#define PDTA_DestMode DTA_Dummy+251

extern ULONG *FindMethod(ULONG *methods, ULONG searchmethodid);
extern struct DTMethod *FindTriggerMethod(struct DTMethod *methods, STRPTR command, ULONG method);

#endif /* __AROS__ */

