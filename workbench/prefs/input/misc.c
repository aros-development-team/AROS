/*
    Copyright © 2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdio.h>

#include "misc.h"

void ShowMessage(CONST_STRPTR msg)
{
    struct EasyStruct es;

    if (msg)
    {
        if (IntuitionBase)
        {
            es.es_StructSize   = sizeof(es);
            es.es_Flags        = 0;
            es.es_Title        = (CONST_STRPTR) "Input";
            es.es_TextFormat   = (CONST_STRPTR) msg;
            es.es_GadgetFormat = "OK";

            EasyRequestArgs(NULL, &es, NULL, NULL); /* win=NULL -> wb screen */
        }
        else
        {
            printf("Input: %s\n", msg);
        }
    }
}

