/*
    Copyright (C) 2004-2016, The AROS Development Team. All rights reserved.

    Desc: File Identifier/starter
*/
/******************************************************************************


    NAME

        Identify

    SYNOPSIS

        FILE/M/A, VERBOSE/S

    LOCATION

        C:

    FUNCTION

        Identifies the file type or directory

    INPUTS

        FILE     --  file to be recognized
        VERBOSE  --  activates verbose output

    RESULT

    NOTES

    EXAMPLE

        > Identify S:Startup-Sequence
        S:Startup-Sequence:     Text/ascii

        (This will identify the startup-sequence as a text file.)

    BUGS

    SEE ALSO

        AddDatatypes

    INTERNALS

    HISTORY

******************************************************************************/

#ifdef __VBCC__
typedef unsigned long IPTR;
#else
#include <proto/alib.h>
#endif
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <datatypes/datatypes.h>

#ifndef BNULL
#define BNULL NULL
#define AROS_LONG2BE
#define ErrorOutput() Output()
#endif

#define PROG_NAME "Identify"
static const TEXT prog_name[] = PROG_NAME;

const TEXT version_string[] = "$VER: " PROG_NAME " 41.2 (14.7.2016)";
static const TEXT template[] = "FILE/M/A,VERBOSE/S";

enum
{
    ARG_FILE,
    ARG_VERBOSE,
    ARG_COUNT
};

/*** Prototypes *************************************************************/
static LONG identify(CONST_STRPTR filename, BOOL verbose);

/*** Functions **************************************************************/
int main(void)
{
    int            rc              = RETURN_OK;
    struct RDArgs *rdargs          = NULL;
    IPTR           args[ARG_COUNT] = { 0 };

    if ((rdargs = ReadArgs(template, args, NULL)) != NULL)
    {
        CONST_STRPTR *files = (CONST_STRPTR *) args[ARG_FILE], file;

        while ((file = *files++) != NULL)
        {
            if (identify(file, args[ARG_VERBOSE]) != RETURN_OK)
            {
                rc = RETURN_WARN;
            }
        }
        
        FreeArgs(rdargs);
    }
    else
    {
        PrintFault(IoErr(), prog_name);
        rc = RETURN_FAIL;
    }

    return rc;
}

static LONG identify(CONST_STRPTR filename, BOOL verbose)
{
    int  rc   = RETURN_OK;
    BPTR lock = Lock(filename, ACCESS_READ);
    
    if (lock != BNULL)
    {
        struct DataType *dt = ObtainDataType(DTST_FILE, (APTR)lock, TAG_DONE);
        if (dt != NULL)
        {
            struct DataTypeHeader *dth = dt->dtn_Header;
            CONST_STRPTR gid_str = GetDTString(dth->dth_GroupID);

            if (!verbose)
            {
                Printf("%s:\t%s/%s\n", filename, gid_str, dth->dth_Name);
            }
            else
            {
                ULONG gid = AROS_LONG2BE(dth->dth_GroupID),
                      id  = AROS_LONG2BE(dth->dth_ID);
                
                Printf
                (
                    "File:        %s\n"
                    "Type:        %s/%s\t(GID: %.4s, ID: %.4s)\n"
                    "DT Basename: %s\n\n",
                    filename, gid_str, dth->dth_Name,
                    (CONST_STRPTR) &gid, (CONST_STRPTR) &id,
                    dth->dth_BaseName
                );
            }

            ReleaseDataType(dt);
        }
        else
        {
            FPrintf(ErrorOutput(),
                PROG_NAME ": Could not obtain datatype for file.\n");
            rc = RETURN_FAIL;
        }

        UnLock(lock);
    }
    else
    {
        PrintFault(IoErr(), prog_name);
        rc = RETURN_FAIL;
    }

    return rc;
}
