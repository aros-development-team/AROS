/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#ifndef _RESOURCES_LOG_H
#define _RESOURCES_LOG_H

struct LogProviderNode
{
    struct Node    lpn_Node;
};

#define EHMB_START                      (1)
#define EHMB_STOP                       (2)
#define EHMB_ADDENTRY                   (3)
#define EHMB_REMENTRY                   (4)

#define LLF_READ                        (1)
#define LLF_WRITE                       (2)

#define LOGEntry_First                  (0)
#define LOGEntry_Last                   (-1)

#define LOGMA_TAGBASE                   (0x88080000)
#define LOGMA_Flags                     (LOGMA_TAGBASE | 1)

#define LOGM_Flag_LevelMask            	0xFF
#define LOGMS_Flag_Type                 8
#define LOGM_Flag_TypeMask            	(0xFF << LOGMS_Flag_Type)

#define LOGMA_DateStamp                	(LOGMA_TAGBASE | 2)
#define LOGMA_EventID                   (LOGMA_TAGBASE | 3)
#define LOGMA_Origin                    (LOGMA_TAGBASE | 4)
#define LOGMA_Component                	(LOGMA_TAGBASE | 5)
#define LOGMA_SubComponent            	(LOGMA_TAGBASE | 6)
#define LOGMA_LogTag                    (LOGMA_TAGBASE | 7)
#define LOGMA_Entry                     (LOGMA_TAGBASE | 8)

#define LOGB_Type_Error                 0
#define LOGF_Type_Error                 (1 << LOGB_Type_Error)
#define LOGF_Flag_Type_Error            (LOGF_Type_Error << LOGMS_Flag_Type)
#define LOGB_Type_Crit                  1
#define LOGF_Type_Crit                  (1 << LOGB_Type_Crit)
#define LOGF_Flag_Type_Crit             (LOGF_Type_Crit << LOGMS_Flag_Type)
#define LOGB_Type_Warn                  2
#define LOGF_Type_Warn                  (1 << LOGB_Type_Warn)
#define LOGF_Flag_Type_Warn             (LOGF_Type_Warn << LOGMS_Flag_Type)
#define LOGB_Type_Information           3
#define LOGF_Type_Information           (1 << LOGB_Type_Information)
#define LOGF_Flag_Type_Information      (LOGF_Type_Information << LOGMS_Flag_Type)
#define LOGB_Type_Verbose               4
#define LOGF_Type_Verbose               (1 << LOGB_Type_Verbose)
#define LOGF_Flag_Type_Verbose          (LOGF_Type_Verbose << LOGMS_Flag_Type)
#define LOGB_Type_Debug                 5
#define LOGF_Type_Debug                 (1 << LOGB_Type_Debug)
#define LOGF_Flag_Type_Debug            (LOGF_Type_Debug << LOGMS_Flag_Type)

#endif /* _RESOURCES_LOG_H */
