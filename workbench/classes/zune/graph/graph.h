#ifndef ZUNE_GRAPH_H
#define ZUNE_GRAPH_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id: $
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Graph                      "Graph.mcc"

/*** Identifier base ********************************************************/
#define MUIB_Graph                      (MUIB_AROS | 0x00000000)

/*** Attributes *************************************************************/
#define MUIA_Graph_InfoText	        (MUIB_Graph | 0x00000001) /* ---  CONST_STRPTR          */
#define MUIA_Graph_EntryCount           (MUIB_Graph | 0x00000002) /* ---  IPTR                  */
#define MUIA_Graph_UpdateInterval       (MUIB_Graph | 0x00000004) /* ---  IPTR                  */
#define MUIA_Graph_Max                  (MUIB_Graph | 0x00000005) /* ---  IPTR                  */
#define MUIA_Graph_Aggregate            (MUIB_Graph | 0x00000006) /* ---  BOOL                  */
#define MUIA_Graph_PeriodicTick  	(MUIB_Graph | 0x000000FF) /* ---  BOOL                  */

/*** Methods ****************************************************************/
#define MUIM_Graph_GetSourceHandle    	(MUIB_Graph | 0x00000001)
#define MUIM_Graph_SetSourceAttrib    	(MUIB_Graph | 0x00000002)
#define MUIM_Graph_Reset    	        (MUIB_Graph | 0x0000000F)
#define MUIM_Graph_Timer    	        (MUIB_Graph | 0x000000FF)

struct MUIP_Graph_GetSourceHandle {STACKED ULONG MethodID; STACKED ULONG SourceNo;};
struct MUIP_Graph_SetSourceAttrib {STACKED ULONG MethodID; STACKED APTR SourceHandle; STACKED ULONG Attrib; STACKED IPTR AttribVal;};

/*** Source Attributes ******************************************************/
#define MUIV_Graph_Source_ReadHook      (0x00000001)
#define MUIV_Graph_Source_Pen           (0x00000002)
#define MUIV_Graph_Source_FillPen       (0x00000003)

/*** Macros *****************************************************************/
#define GraphObject MUIOBJMACRO_START(MUIC_Graph)

#endif /* ZUNE_GRAPH_H */
