#ifndef __CALLTIPS_MCC_H__
#define __CALLTIPS_MCC_H__

/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIC_Calltips  "Calltips.mcc"

#define TAGBASE_Calltips                    0xfecf4100

#define MUIA_Calltips_Rectangle             (TAGBASE_Calltips + 0)
#define MUIA_Calltips_Layout                (TAGBASE_Calltips + 1)
#define MUIA_Calltips_Source                (TAGBASE_Calltips + 2)
#define MUIA_Calltips_MarginLeft            (TAGBASE_Calltips + 4)
#define MUIA_Calltips_MarginTop             (TAGBASE_Calltips + 5)

#define MUIV_Calltips_Layout_Exact          0

#define MUIM_Calltips_SetRectangle          (TAGBASE_Calltips + 0)
#define MUIM_Calltips_ParentSetup           (TAGBASE_Calltips + 1)
#define MUIM_Calltips_ParentCleanup         (TAGBASE_Calltips + 2)
#define MUIM_Calltips_ParentShow            (TAGBASE_Calltips + 3)
#define MUIM_Calltips_ParentHide            (TAGBASE_Calltips + 4)
#define MUIM_Calltips_ParentWindowArranged  (TAGBASE_Calltips + 5)

struct MUIP_Calltips_SetRectangle { STACKED ULONG id; STACKED LONG MinX; STACKED LONG MinY; STACKED LONG MaxX; STACKED LONG MaxY;};

#endif  /* __CALLTIPS_MCC_H__ */
