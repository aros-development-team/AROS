#ifndef SUPPORT_H
#    define SUPPORT_H

#    include <intuition/classusr.h>

struct DesktopInternMsg
{
    struct Message  di_Message;
    ULONG           di_Command; /* see below */
};

struct HandlerScanRequest
{
    struct DesktopInternMsg hsr_Message;
    BPTR            hsr_DirLock;
    Object         *hsr_CallBack;
    Object         *hsr_Application;
};

struct HandlerTopLevelRequest
{
    struct DesktopInternMsg htl_Message;
    ULONG           htl_Types;
    Object         *htl_CallBack;
    Object         *htl_Application;
};

#    define DIMC_ADDUSER        10
#    define DIMC_SUBUSER        20
#    define DIMC_SCANDIRECTORY  30
#    define DIMC_TOPLEVEL       40

struct WorkingMessageNode
{
    struct MinNode  wm_Node;
    struct DesktopInternMsg *wm_Working;
    ULONG           wm_ID;
    struct MsgPort *wm_Port;
};

/*
   Kinds for CreateDesktopObjectA 
 */

#    define CDO_Window                        1
#    define CDO_IconContainer                 2
#    define CDO_DiskIcon                      3
#    define CDO_DrawerIcon                    4
#    define CDO_ToolIcon                      5
#    define CDO_ProjectIcon                   6
#    define CDO_TrashcanIcon                  7
#    define CDO_Desktop                       8

#endif
