#ifndef __DYNMODULE_INTERN_H
#define __DYNMODULE_INTERN_H

/*
 * Internal definitions used to implement the dynamic module support
 */

#include <exec/exec.h>
#include <dynmod/dynmodule.h>

typedef enum
{
    DMIFERR_Ok                      = 0,
    DMIFERR_Closing,
    DMIFERR_StackNotSupported,
    DMIFERR_OutOfMemory
} __dynmoduleiferror_t;

typedef enum
{
    DMIFMSG_Open                    = 0,
    DMIFMSG_Close,
    DMIFMSG_Dispose,
    DMIFMSG_Resolve,
    DMIFMSG_TYPEMAX
} __dynmodulemsgtype_t;

typedef struct {
    dynmod_stackf_t         StackType;
    __dynmoduleiferror_t    Error;
} dynmodi_ifopenmsg_t;

typedef struct {
} dynmodi_ifclosemsg_t;

/*
*/
typedef struct
{
    struct Message                  Message;
    __dynmodulemsgtype_t            IFMsgType;
    union 
    {
        dynmodi_ifopenmsg_t         IFOpenRequest;
        dynmodi_ifclosemsg_t        IFCloseRequest;
        dynmod_sym_t                IFResolveRequest;
    };
} __dynmodulemsg_t;

typedef struct
{
    struct MsgPort              *dmi_IFMsgPort;
    dynmod_stackf_t             dmi_StackFType;
    dynmoduleLoadResourceFn_t   dmi_LoadResFunc;
    dynmoduleFreeResourceFn_t   dmi_FreeResFunc;
    dynmoduleFindResourceFn_t   dmi_FindResFunc;
} __dynmoduleinstance_t;

/************************************************************
 * Misc
 ************************************************************/

extern void *dynmodule__InternalLoadModule(const char *, const char *, BOOL);
extern void dynmodule__InternalFreeModule(int);
extern void dynmodule__InternalCleanup();

#endif
