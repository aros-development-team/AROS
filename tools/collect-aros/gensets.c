/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
    This routine is slow, but does the work and it's the simplest to write down.
    All this will get integrated into the linker anyway, so there's no point
    in doing optimizations
*/

int gensets(FILE *in, FILE *out)
{
    char secbuf[201];
    char setname_big[201];
    char *secname = secbuf;
    char *setname;
    int  i;
    int  have_ctors = 0;
    int  have_dtors = 0;

    while (fscanf(in, " %200s ", secname)>0)
    {
        char *idx;
	int   i, have_set = 0;
        int   is_ctors = 0, is_dtors = 0;

        if (strncmp(secname, ".aros.set.", 10) == 0)
        {
            have_set  = 1;
            secname  += 9;
        }

        if (strncmp(secname, ".ctors", 5) == 0)
	{
            if (have_ctors)
                continue;

            have_ctors = is_ctors = 1;
        }
        else
        if (strncmp(secname, ".dtors", 5) == 0)
        {
            if (have_dtors)
                continue;

            have_dtors = is_dtors = 1;
        }
        else
        if (!have_set)
            continue;

        setname = secname + 1;

        idx = strchr(setname, '.');
        if (idx)
            *idx = '\0';

        i = 0;
        do
        {
            setname_big[i] = toupper(setname[i]);
        } while (setname[i++]);


        fprintf
        (
            out,
            "    __%s_LIST__ = .;\n"
            "    LONG((__%s_END__ - __%s_LIST__) / %d - 2)\n"
            "%s"
            "    KEEP(*(SORT(.aros.set.%s.*)))\n"
            "    KEEP(*(.aros.set.%s))\n"
            "%s"
            "    LONG(0)\n"
            "    __%s_END__ = .;\n",
	    setname_big,
            setname_big, setname_big, sizeof(long),
            is_dtors?
                "    KEEP(*(SORT(.dtors.*)))\n"
                "    KEEP(*(.dtors))\n": "",
            setname,
            setname,
            is_ctors?
                "    KEEP(*(SORT(.ctors.*)))\n"
                "    KEEP(*(.ctors))\n": "",
            setname_big
        );
    }

    return 1;
}
