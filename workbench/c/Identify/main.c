/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <proto/datatypes.h>
#include <datatypes/datatypes.h>

#include <stdio.h>

enum
{
    ARG_FILE,
    ARG_VERBOSE,
    ARG_COUNT
};

#define ERROR_HEADER "Identify"

/*** Prototypes *************************************************************/
int          identify(CONST_STRPTR filename, BOOL verbose);
CONST_STRPTR gid2string(ULONG gid);

/*** Functions **************************************************************/
int main(void)
{
    int            rc              = RETURN_OK;
    struct RDArgs *rdargs          = NULL;
    IPTR           args[ARG_COUNT] = { 0 };

    if ((rdargs = ReadArgs("FILE/M/A,VERBOSE/S", args, NULL)) != NULL)
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
        PrintFault(IoErr(), ERROR_HEADER);
        rc = RETURN_FAIL;
    }

    return rc;
}

int identify(CONST_STRPTR filename, BOOL verbose)
{
    int  rc   = RETURN_OK;
    BPTR lock = Lock(filename, ACCESS_READ);
    
    if (lock != NULL)
    {
        struct DataType *dt = ObtainDataType(DTST_FILE, lock, TAG_DONE);
        if (dt != NULL)
        {
            struct DataTypeHeader *dth = dt->dtn_Header;
            
            if (!verbose)
            {
                Printf
                (
                    "%s:\t%s/%s\n", 
                    filename, gid2string(dth->dth_GroupID), dth->dth_Name
                );
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
                    filename, gid2string(dth->dth_GroupID), dth->dth_Name,
                    (CONST_STRPTR) &gid, (CONST_STRPTR) &id,
                    dth->dth_BaseName
                );
            }
        }
        else
        {
            FPrintf(Error(), ERROR_HEADER": Could not obtain datatype for file.\n");
            rc = RETURN_FAIL;
        }

        UnLock(lock);
    }
    else
    {
        PrintFault(IoErr(), ERROR_HEADER);
        rc = RETURN_FAIL;
    }

    return rc;
}

CONST_STRPTR gid2string(ULONG gid)
{
    switch (gid)
    {
        case GID_SYSTEM:     return "System";
        case GID_TEXT:       return "Text";
        case GID_DOCUMENT:   return "Document";
        case GID_SOUND:      return "Sound";
        case GID_INSTRUMENT: return "Instrument";
        case GID_MUSIC:      return "Music";
        case GID_PICTURE:    return "Picture";
        case GID_ANIMATION:  return "Animation";
        case GID_MOVIE:      return "Movie";
        default:             return NULL;
    }
}
