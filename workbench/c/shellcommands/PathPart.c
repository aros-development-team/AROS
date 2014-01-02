/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        PathPart

    TEMPLATE

        DIR/K,FILE/K,ADD/K/M

    LOCATION

        C:

    FUNCTION

        Extracts directory or file name from a path, or assembles a path

    FORMAT

        PATHPART [DIR <path name>] [FILE <path name>] [ADD {<device name | directory name | file name>}]

    RESULT

        Standard DOS return codes.


******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>

//#define DEBUG 1
#include <aros/debug.h>

#include <aros/shcommands.h>

#define MAX_PATH_LEN 2048 /* Same as in C:Delete */

AROS_SH3H(PathPart,50.1, "extract directory or file name from a path, or assemble a path\n",
AROS_SHAH(STRPTR  ,,DIR ,/K  ,NULL,"Path from which to extract the directory name"),
AROS_SHAH(STRPTR  ,,FILE,/K  ,NULL,"Path from which to extract the file name"),
AROS_SHAH(STRPTR *,,ADD ,/K/M,NULL,"Device and/or directory(ies) and/or file names from\n"
                                 "\twhich to build a path (order matters!)\n") )
{
    AROS_SHCOMMAND_INIT

    int    rc     = RETURN_FAIL;
    STRPTR outstr = NULL, buf = NULL;
    UBYTE  buf2[MAX_PATH_LEN];
    LONG   size;

    if (    (SHArg(ADD) && SHArg(DIR))  
         || (SHArg(ADD) && SHArg(FILE))
         || (SHArg(DIR) && SHArg(FILE))
       )
    {
        SetIoErr(ERROR_BAD_TEMPLATE);
    }
    else if (SHArg(DIR))
    {
        size  = strlen((char *)SHArg(DIR));
        if ((buf = AllocVec(size, MEMF_ANY)))
        {
            CopyMem (SHArg(DIR), buf, size);
            buf[PathPart(buf) - buf] = '\0';
            outstr = buf;
            rc = RETURN_OK;
        }
    }
    else if (SHArg(FILE))
    {
        outstr = FilePart(SHArg(FILE));
        rc = RETURN_OK;
    }
    else if (SHArg(ADD))
    {
        buf2[0]='\0';
        while (*SHArg(ADD))
        {
            if (!AddPart(buf2 ,*SHArg(ADD)++ ,MAX_PATH_LEN))
            {
                /* rc == RETURN_FAIL until this point... */
                rc = RETURN_ERROR;
                break;
            }
        }
        if (rc != RETURN_ERROR)
        {
            outstr = buf2;
            rc = RETURN_OK;
        }
    }
    else
        SetIoErr(ERROR_REQUIRED_ARG_MISSING);

    if (rc == RETURN_OK)
    {
        if (outstr)
        {
            PutStr(outstr);
            FPutC(Output(), '\n');
        }
    }
    else
        PrintFault(IoErr(), (CONST_STRPTR)"PathPart");

    if (buf)
        FreeVec(buf);

    return rc;

    AROS_SHCOMMAND_EXIT
}


