/*
    Copyright © 2012-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Play

    SYNOPSIS

        FILE/A

    LOCATION

        C:

    FUNCTION

        Play a sound file, using datatypes.library

    INPUTS

        FILE f   --  Filename to play

    RESULT

        Sound should play to the default audio device

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/datatypes.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/soundclass.h>

#include <aros/shcommands.h>

static inline BOOL isPlayable(struct Library *DataTypesBase, Object *obj)
{
    struct DTMethod *dtm;

    for (dtm = GetDTTriggerMethods(obj); dtm && dtm->dtm_Label; dtm++) {
        if (dtm->dtm_Method == STM_PLAY)
            return TRUE;
    }

    return FALSE;
}

AROS_SH1H(Play, 44.1, "Play a sound file using DataTypes\n",
AROS_SHAH(STRPTR, , FILE, /A, NULL, "File to play\n"))
{
    AROS_SHCOMMAND_INIT

    LONG result = RETURN_FAIL;
    struct Library *DataTypesBase;
    STRPTR file = SHArg(FILE);

    if ((DataTypesBase = OpenLibrary("datatypes.library", 0))) {
        Object *o;
        if ((o = NewDTObject(file, SDTA_SignalTask, FindTask(NULL),
            SDTA_SignalBitNumber, SIGB_SINGLE, TAG_END))) {
            if (isPlayable(DataTypesBase, o)) {
                struct dtTrigger msg;
                msg.MethodID          = DTM_TRIGGER;
                msg.dtt_GInfo         = NULL;
                msg.dtt_Function      = STM_PLAY;
                msg.dtt_Data          = NULL;
                if (0 == DoDTMethodA(o, NULL, NULL, (Msg)&msg)) {
                    Wait(SIGF_SINGLE | SIGBREAKF_CTRL_C);
                    result = RETURN_OK;
                } else {
                    Printf("Can't play \"%s\"\n", file);
                    result = RETURN_FAIL;
                }
            } else {
                Printf("\"%s\" is not a DataType playable sound file\n", file);
                result = RETURN_FAIL;
            }
            DisposeDTObject(o);
        } else {
            Printf("Can't open %s as a DataType object\n", file);
        }
        CloseLibrary(DataTypesBase);
    } else {
        Printf("Can't open datatypes.library\n");
    }

    return result;

    AROS_SHCOMMAND_EXIT
}
