/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$

    AltKeyQ -- Enter characters by their ANSI number.
*/

/******************************************************************************

    NAME

        AltKeyQ

    SYNOPSIS

        CX_PRIORITY/N/K

    LOCATION

        SYS:Tools/Commodities

    FUNCTION

        Enter characters by pressing the ALT key and ANSI number of
        the character. It's a clone of a Commodity which you can
        find under the same name in Aminet.

    INPUTS

        CX_PRIORITY  --  The priority of the commodity

    RESULT

    NOTES

    EXAMPLE

        <Alt> 1 2 0 inserts 'x' into the input stream.

    BUGS

        You can only enter characters which are defined
        in the keymap of your keyboard.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <devices/rawkeycodes.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/commodities.h>
#include <proto/alib.h>

const char *verstag = "\0$VER: AltKeyQ 1.0 (23.05.2009) © The AROS Development Team";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

static struct NewBroker nb =
{
    NB_VERSION,
    NULL,
    NULL,
    NULL,
    NBU_NOTIFY | NBU_UNIQUE,
    0,
    0,
    NULL,                             
    0 
};

struct AKQState
{
    CxObj          *akq_broker;
    struct MsgPort *akq_msgPort;
};

struct  /* structure with information to send: */
{
    UBYTE value;        /*     ASCII value for the IEvent to send */
    UBYTE nul;          /*     ASCII NUL byte for InvertString() */
} send;

enum {
    ARG_PRI,
    NUM_ARGS
};

static struct Catalog *catalog;
static struct Task *mainTask;
static ULONG sendSigBit = -1;

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  3

/************************************************************************************/

static void collectKeysFunc(CxMsg *msg, CxObj *co);
static void handleCx(struct AKQState *as);
static void freeResources(struct AKQState *as);
static BOOL initiate(int argc, char **argv, struct AKQState *as);
static void showSimpleMessage(CONST_STRPTR msgString);
static VOID Locale_Deinitialize(VOID);
static BOOL Locale_Initialize(VOID);
static CONST_STRPTR _(ULONG id);

/************************************************************************************/

static CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
        return CatCompArray[id].cca_Str;
    }
}

/************************************************************************************/

static BOOL Locale_Initialize(VOID)
{
    if (LocaleBase != NULL)
    {
        catalog = OpenCatalog(NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE);
    }
    else
    {
        catalog = NULL;
    }

    return TRUE;
}

/************************************************************************************/

static VOID Locale_Deinitialize(VOID)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}

/************************************************************************************/

static void showSimpleMessage(CONST_STRPTR msgString)
{
    struct EasyStruct easyStruct;

    easyStruct.es_StructSize	= sizeof(easyStruct);
    easyStruct.es_Flags		= 0;
    easyStruct.es_Title		= _(MSG_ALTKEYQ_CXNAME);
    easyStruct.es_TextFormat	= msgString;
    easyStruct.es_GadgetFormat	= _(MSG_OK);		

    if (IntuitionBase != NULL && !Cli() )
    {
        EasyRequestArgs(NULL, &easyStruct, NULL, NULL);
    }
    else
    {
        PutStr(msgString);
    }
}

/************************************************************************************/

static BOOL initiate(int argc, char **argv, struct AKQState *as)
{
    CxObj *customObj;

    memset(as, 0, sizeof(struct AKQState));

    if (Cli() != NULL)
    {
        struct RDArgs *rda;
        IPTR          *args[] = { NULL, (IPTR)FALSE };

        rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);

        if (rda != NULL)
        {
            if (args[ARG_PRI] != NULL)
            {
                nb.nb_Pri = *args[ARG_PRI];
            }
        }
        FreeArgs(rda);
    }
    else
    {
        UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);

        nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);

        ArgArrayDone();
    }

    nb.nb_Name = _(MSG_ALTKEYQ_CXNAME);
    nb.nb_Title = _(MSG_ALTKEYQ_CXTITLE);
    nb.nb_Descr = _(MSG_ALTKEYQ_CXDESCR);

    as->akq_msgPort = CreateMsgPort();

    if (as->akq_msgPort == NULL)
    {
        showSimpleMessage(_(MSG_CANT_CREATE_MSGPORT));
        return FALSE;
    }

    nb.nb_Port = as->akq_msgPort;

    as->akq_broker = CxBroker(&nb, 0);

    if (as->akq_broker == NULL)
    {
        return FALSE;
    }

    customObj = CxCustom(collectKeysFunc, 0);

    if (customObj == NULL)
    {
        showSimpleMessage(_(MSG_CANT_CREATE_CUSTOM));
        return FALSE;
    }

    AttachCxObj(as->akq_broker, customObj);
    sendSigBit = AllocSignal(-1);
    if (sendSigBit == -1)
    {
        showSimpleMessage(_(MSG_CANT_ALLOCATE_SIGNAL));
        return FALSE;
    }
    mainTask = FindTask(NULL);
    ActivateCxObj(as->akq_broker, TRUE);

    return TRUE;
}

/************************************************************************************/

static void freeResources(struct AKQState *as)
{
    struct Message  *cxm;

    if (CxBase != NULL)
    {
        if (as->akq_broker != NULL)
        {
            DeleteCxObjAll(as->akq_broker);
        }
    }

    if (as->akq_msgPort != NULL)
    {
        while ((cxm = GetMsg(as->akq_msgPort)))
        {
            ReplyMsg(cxm);
        }

        DeleteMsgPort(as->akq_msgPort);
    }

    FreeSignal(sendSigBit);
}

/************************************************************************************/

static void collectKeysFunc(CxMsg *msg, CxObj *co)
{
    /* Scancodes of numeric pad */
    static TEXT keys[]= "\x0f\x1d\x1e\x1f\x2d\x2e\x2f\x3d\x3e\x3f";

    static BOOL collflag;
    static ULONG value;
    TEXT *s;

    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    if (ie->ie_Class == IECLASS_RAWKEY)
    {
        if (ie->ie_Code == (RAWKEY_LALT | IECODE_UP_PREFIX))
        {
            if (collflag)
            {
                /* User released left ALT key */
                if (value < 256)
                {
                    send.value = value;
                    D(bug("Value %u Character %c\n", send.value, send.value));
                    Signal(mainTask, 1 << sendSigBit);
                }
                else
                {
                    D(bug("Value too large\n"));
                }

                goto setinactive;
            }
        }
        else if (ie->ie_Qualifier & IEQUALIFIER_LALT)
        {
            if ((s= strchr(keys, ie->ie_Code)))
            {
                /* collect value */
                ie->ie_Code |= IECODE_UP_PREFIX;
                value = value * 10 + (s - keys);
                collflag = TRUE;
            }
        }
        else
setinactive:
        {
            value = 0;
            collflag = FALSE;
        }
    }
}

/************************************************************************************/

/* React on command messages sent by commodities.library */
static void handleCx(struct AKQState *as)
{
    CxMsg *msg;
    BOOL   quit = FALSE;
    LONG   signals;

    while (!quit)
    {
        signals = Wait((1 << nb.nb_Port->mp_SigBit) | (1 << sendSigBit) | SIGBREAKF_CTRL_C);
        
        if (signals & (1 << sendSigBit))
        {
            D(bug("signal received\n"));
            struct InputEvent *ie = InvertString((TEXT *)&send, NULL);
            if (ie)
            {
                AddIEvents(ie);
                FreeIEvents(ie);
            }
            else
            {
                D(bug("No event added\n"));
            }
        }

        if (signals & (1 << nb.nb_Port->mp_SigBit))
        {
            while ((msg = (CxMsg *)GetMsg(as->akq_msgPort)))
            {
                switch (CxMsgType(msg))
                {
                    case CXM_COMMAND:
                        switch (CxMsgID(msg))
                        {
                            case CXCMD_DISABLE:
                                ActivateCxObj(as->akq_broker, FALSE);
                                break;

                            case CXCMD_ENABLE:
                                ActivateCxObj(as->akq_broker, TRUE);
                                break;

                            case CXCMD_UNIQUE:
                                /* Running the program twice <=> quit */
                                /* Fall through */

                            case CXCMD_KILL:
                                quit = TRUE;
                                break;

                        } /* switch(CxMsgID(msg)) */

                        break;
                } /* switch (CxMsgType(msg))*/

                ReplyMsg((struct Message *)msg);

            } /* while((msg = (CxMsg *)GetMsg(cs->cs_msgPort))) */
        }	    

        if (signals & SIGBREAKF_CTRL_C)
        {
            quit = TRUE;
        }

    } /* while (!quit) */
}

/************************************************************************************/

int main(int argc, char **argv)
{
    struct AKQState akqState;
    int     error = RETURN_OK;

    if (initiate(argc, argv, &akqState))
    {
        handleCx(&akqState);
    }
    else
    {
        error = RETURN_FAIL;
    }

    freeResources(&akqState);

    return error;
}

/************************************************************************************/

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);

