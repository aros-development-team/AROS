/*
 * This file contains glue linked into the dynamic-module
 */

//#define DEBUG 1
#include <aros/debug.h>

#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "dynmodule_intern.h"

/*
 * symbols provided by the linked module ...
 */
__attribute__((weak)) const char *DYNMODULE_Name = NULL;
extern int DYNMODULE_Setup(APTR);
extern void DYNMODULE_Cleanup(void);

/*
 * glue function implementations ...
 */
APTR __dynglue_FindResource(int id, const char *pType)
{
    return NULL;
}

APTR __dynglue_LoadResource(APTR handle)
{
    return NULL;
}

void __dynglue_FreeResource(APTR handle)
{
    return;
}

/*
 * internal glue functions ...
 */
struct MsgPort *__dynglue_InitIFPort(char *name)
{
    struct MsgPort *ifMPort = NULL;

    ifMPort = CreatePort(name, 0);

    return ifMPort;
}

__dynmodulemsg_t *__dynglue_GetIFMSg(struct MsgPort *dmmport, BOOL inuse)
{
    __dynmodulemsg_t *ifMsg = NULL;
    if (inuse) {
        WaitPort(dmmport);
        ifMsg = (__dynmodulemsg_t *)GetMsg(dmmport);
    }
    return ifMsg;
}

__dynmodulemsg_t *__dynglue_DisposeIFPort(struct MsgPort *dmmport)
{
    __dynmodulemsg_t *ifMsg;
    while((ifMsg = (__dynmodulemsg_t *)GetMsg(dmmport))) {
        ifMsg->IFOpenRequest.Error = DMIFERR_Closing;
        ReplyMsg((struct Message *)ifMsg);
    }
    if (dmmport->mp_Node.ln_Name)
        FreeVec(dmmport->mp_Node.ln_Name);
    DeletePort(dmmport);
}

/*
 * Module entry point ...
 */

#define ARG_TEMPLATE "PORT/A"
enum
{
    ARG_PORT = 0,
    NOOFARGS
};

int main(int argc, char **argv)
{
    IPTR           args[NOOFARGS + 1] = { (IPTR)NULL, (IPTR)0 };
    struct RDArgs *rda;

    struct MsgPort *dmmport;
    __dynmodulemsg_t *ifMsg;
    char *pname;

    int opencnt = 0;
    BOOL inuse = TRUE;

    D(
        const char *dbgname;
        if ((&DYNMODULE_Name == NULL) || (DYNMODULE_Name == NULL))
            dbgname = argv[0];
        else
            dbgname = DYNMODULE_Name;
        bug("[%s] %s('%s')\n", dbgname, __func__, argv[0]);
    )

    if ((rda = ReadArgs(ARG_TEMPLATE, args, NULL)) != NULL) {
        char *argpname = (char *)args[ARG_PORT];
        if (*argpname == '"') {
            char prttmp[255];
            int prtlen = strlen(argpname) - 2;
            strncpy(prttmp, &argpname[1], prtlen);
            prttmp[prtlen] = 0;
            pname = StrDup((CONST_STRPTR)prttmp);
        }
        else
            pname = StrDup((CONST_STRPTR)args[ARG_PORT]);
        FreeArgs(rda);
    }
    if (!pname) {
        if ((&DYNMODULE_Name == NULL) || (DYNMODULE_Name == NULL))
            pname = StrDup((CONST_STRPTR)argv[0]);
        else
            pname = StrDup(DYNMODULE_Name);
    }

    dmmport = __dynglue_InitIFPort(pname);
    if (!dmmport)
        DYNMODULE_Exit(0);

    D(
        bug("[%s] %s: port '%s' @ 0x%p\n", dbgname, __func__,
            dmmport->mp_Node.ln_Name, dmmport);
    )

    if(!DYNMODULE_Setup(dmmport)) {
        __dynglue_DisposeIFPort(dmmport);
        DYNMODULE_Exit(0);
    }

    D(bug("[%s] %s: initialised\n", dbgname, __func__));

    while((ifMsg = __dynglue_GetIFMSg(dmmport, inuse))) {
        if (ifMsg) {
            switch(ifMsg->IFMsgType) {
            case DMIFMSG_Open:
                    D(bug("[%s] %s: DMIFMSG_Open\n", dbgname, __func__));
                    if (++opencnt > 0)
                        inuse = TRUE;
                    ifMsg->IFOpenRequest.Error = DMIFERR_Ok;
                    break;

            case DMIFMSG_Close:
                    D(bug("[%s] %s: DMIFMSG_Close\n", dbgname, __func__));
                    if (--opencnt <= 0)
                        inuse = FALSE;
                    break;

            case DMIFMSG_Dispose:
                    D(bug("[%s] %s: DMIFMSG_Dispose\n", dbgname, __func__));
                    inuse = FALSE;
                    break;

            case DMIFMSG_Resolve:
                    D(bug("[%s] %s: DMIFMSG_Resolve\n", dbgname, __func__));
                    dynmoduleExport(&ifMsg->IFResolveRequest);
                    break;

            default:
                    D(bug("[%s] %s: Unknown Msg type received!\n", dbgname, __func__));
                    break;
            }
            ReplyMsg((struct Message *)ifMsg);
        }
    }

    __dynglue_DisposeIFPort(dmmport);
    DYNMODULE_Cleanup();

    return 0;
}
