/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generate UUIDs
    Lang: English
*/

/******************************************************************************

    NAME

        UUIDGen

    SYNOPSIS

        RANDOM/S, AMOUNT/N

    LOCATION

        Sys:C

    FUNCTION

        Generates one or more universally unique identifiers. They may be either 
        time-based or random. Please note that the quality of random generated 
        uuid's may be poor, due to the lack of high-quality noise generators on AROS.

    INPUTS

        RANDOM --  Generate randob-based UUID instead of time based
        AMOUNT --  Amount of UUID's to generate. Defaults to 1.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/uuid.h>
#include <libraries/uuid.h>
#include <stdio.h>

#define  ARG_COUNT  2    /* Number of ReadArgs() arguments */

int __nocommandline;

int main(void)
{
    ULONG amount = 1;
    /* Array filled by ReadArgs() call */
    IPTR args[ARG_COUNT] = {0, 0};
    
    struct RDArgs *rda;           /* ReadArgs standard struct */
    
    if((rda = ReadArgs("RANDOM/S,AMOUNT/N", args, NULL)) != NULL)
    {
        BOOL random = (BOOL)args[0];    /* Random-based UUID? */
        
        if (args[1])
            amount = *(ULONG*)args[1];  /* Amount of uuid's was given. use it */
            
        struct Library *UUIDBase = OpenLibrary("uuid.library", 0);

        if (UUIDBase)
        {
            char uuidstr[UUID_STRLEN+1];
            uuid_t uuid;
            uuidstr[UUID_STRLEN] = 0;

            while(amount--)
            {
                /* Generate the uuid identifier */
                if (random)
                    UUID_Generate(UUID_TYPE_DCE_RANDOM, &uuid);
                else
                    UUID_Generate(UUID_TYPE_DCE_TIME, &uuid);
                
                /* unparse it into human-readable format */
                UUID_Unparse(&uuid, uuidstr);
                
                Printf("%s\n", uuidstr);
            }
            
            CloseLibrary(UUIDBase);
        }
    }
    
    return RETURN_OK;
}
