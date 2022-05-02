#ifndef ZUNE_GRAPH_H
#define ZUNE_GRAPH_H

/*
    Copyright © 2017-2022, The AROS Development Team. All rights reserved.
    $Id: $
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Graph                      "Graph.mcc"

/*** Identifier base ********************************************************/
#define MUIB_Graph                      (MUIB_AROS | 0x00000000)

/*** Attributes *************************************************************/
#define MUIA_Graph_InfoText	            (MUIB_Graph | 0x00000001) /* ---  CONST_STRPTR          */
#define MUIA_Graph_EntryCount           (MUIB_Graph | 0x00000002) /* ---  IPTR                  */
#define MUIA_Graph_Aggregate            (MUIB_Graph | 0x00000003) /* ---  BOOL                  */
#define MUIA_Graph_PeriodCeiling        (MUIB_Graph | 0x00000010) /* ---  IPTR   in ms          */
#define MUIA_Graph_PeriodStep           (MUIB_Graph | 0x00000011) /* ---  IPTR   in ms          */
#define MUIA_Graph_PeriodInterval       (MUIB_Graph | 0x00000012) /* ---  IPTR   in ms          */
#define MUIA_Graph_ValueCeiling         (MUIB_Graph | 0x00000020) /* ---  IPTR                  */
#define MUIA_Graph_ValueStep            (MUIB_Graph | 0x00000021) /* ---  IPTR                  */
#define MUIA_Graph_ManualRefresh  	    (MUIB_Graph | 0x000000FE) /* ---  BOOL                  */
#define MUIA_Graph_PeriodicTick  	    (MUIB_Graph | 0x000000FF) /* ---  BOOL                  */

/*** Methods ****************************************************************/
#define MUIM_Graph_GetSourceHandle    	(MUIB_Graph | 0x00000001)
#define MUIM_Graph_SetSourceAttrib    	(MUIB_Graph | 0x00000002)
#define MUIM_Graph_Reset    	        (MUIB_Graph | 0x0000000F)
#define MUIM_Graph_Timer    	        (MUIB_Graph | 0x000000FF)

struct MUIP_Graph_GetSourceHandle {STACKED ULONG MethodID; STACKED ULONG SourceNo;};
struct MUIP_Graph_SetSourceAttrib {STACKED ULONG MethodID; STACKED APTR SourceHandle; STACKED ULONG Attrib; STACKED IPTR AttribVal;};

/*** Source Attributes ******************************************************/
#define MUIV_Graph_Source_ReadHook      (0x00000001)
#define MUIV_Graph_Source_PenSrc        (0x00000002)
#define MUIV_Graph_Source_Pen           (0x00000003)
#define MUIV_Graph_Source_FillPenSrc    (0x00000004)
#define MUIV_Graph_Source_FillPen       (0x00000005)

/*** Macros *****************************************************************/
#define GraphObject MUIOBJMACRO_START(MUIC_Graph)

#endif /* ZUNE_GRAPH_H */
