/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: $
*/

/******************************************************************************


    NAME

        AutoMount

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

    INPUTS

    RESULT

    NOTES

        Command is called in Startup-Sequence.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <proto/alib.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include <stdlib.h>
#include <string.h>

struct HandlerNode
{
    struct MinNode n;
    ULONG          id;
    ULONG          mask;
    char           handler[1];
};

struct MinList handlerlist;
APTR pool;

LONG parsePrefs(char *buffer, LONG size)
{
    struct CSource csrc = {buffer, size, 0};
    char ident[256];
    LONG res;
    WORD line = 1;

    D(bug("[Automount] parsePrefs()\n"));

    while (csrc.CS_CurChr < csrc.CS_Length)
    {
        struct HandlerNode *tn;
        int i;
        char *p;
        ULONG id = 0;
        ULONG mask = 0xFFFFFFFF;

        DB2(bug("[parsePrefs] Cur %d, Length %d\n", csrc.CS_CurChr, csrc.CS_Length));

        res = ReadItem(ident, 256, &csrc);
        switch (res)
        {
        case ITEM_ERROR:
            return IoErr();

        case ITEM_UNQUOTED:
            if (ident[0] == '#')
            {
            	/* Skip over to the end of line */
                while ((csrc.CS_CurChr < csrc.CS_Length) && (buffer[csrc.CS_CurChr] != '\n'))
		    csrc.CS_CurChr++;

                goto next_line;
            }
            /* Fall through */
        case ITEM_QUOTED:
            p = ident;
            for (i = 0; i < 4; i++)
            { 
                UBYTE c;

                if (!*p)
                {
                    Printf("LINE %ld: Mailformed filesystem ID\n", line);
                    return -1;
                }

                c = *p++;
                switch (c)
                {
                case '\\':
                    c = strtoul(p, &p, 16);
                    break;

                case '?':
                    mask &= ~(0xFF000000 >> (i << 3));
                    c = 0;
                }

                id <<= 8;
                id |= c;
            }
            break;

        default:
            Printf("LINE %ld: Missing filesystem ID\n", line);
            return -1;
        }

        res = ReadItem(ident, 256, &csrc);
        if (res == ITEM_ERROR)
            return IoErr();

        if (res != ITEM_EQUAL)
        {
            Printf("LINE %ld: Unexpected item after filesystem ID\n", line);
            return -1;
        }

        res = ReadItem(ident, 256, &csrc);
        if (res == ITEM_ERROR)
            return IoErr();

        if ((res != ITEM_QUOTED) && (res != ITEM_UNQUOTED))
        {
            Printf("LINE %ld: Missing handler name\n", line);
            return -1;
        }

        res = strlen(ident);
        tn = AllocPooled(pool, sizeof(struct HandlerNode) + res);
        if (tn == NULL)
	    return ERROR_NO_FREE_STORE;
	
	tn->id   = id;
	tn->mask = mask;
        CopyMem(ident, tn->handler, res + 1);

        AddTail((struct List *)&handlerlist, (struct Node *)tn);

next_line:
	/*
	 * Intentional ReadItem() bug workaround.
	 * Ungets '\n' every time, causing an infinite loop without this adjustment.
	 */
        if ((csrc.CS_CurChr < csrc.CS_Length) && (buffer[csrc.CS_CurChr] == '\n'))
        {
            line++;
	    csrc.CS_CurChr++;
	}
    }

    return 0;
}

static LONG LoadPrefs(STRPTR filename) 
{
    struct FileInfoBlock fib;
    char *buffer;
    LONG retval = 0;
    LONG size;
    BPTR fh;

    D(bug("[Automount] LoadPrefs('%s')\n", filename));

    fh = Open(filename, MODE_OLDFILE);
    if (fh)
    {
        if (ExamineFH(fh, &fib))
        {
            if (fib.fib_Size>0)
            {
                buffer = AllocMem(fib.fib_Size, MEMF_PUBLIC | MEMF_CLEAR);
                if (buffer)
                {
                    size = Read(fh, buffer, fib.fib_Size);
                    if (size == fib.fib_Size)
                        retval = parsePrefs(buffer, size);
                    else
                    	retval = IoErr();

                    FreeMem(buffer, fib.fib_Size);
                }
                else
                    retval = ERROR_NO_FREE_STORE;
            }
        }
        else
            retval = IoErr();

        Close(fh);
    }
    
    return retval;
}

static struct HandlerNode *FindHandler(ULONG id)
{
    struct HandlerNode *n;

    ForeachNode(&handlerlist, n)
    {
        if (n->id == (id & n->mask))
            return n;
    }
    
    return NULL;
}

static BOOL IsMounted(struct DeviceNode *dn)
{
    BOOL ret = FALSE;
    struct DosList *dl = LockDosList(LDF_DEVICES|LDF_READ);
    
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
    	if (dl == (struct DosList *)dn)
    	{
    	    ret = TRUE;
    	    break;
    	}
    }

    UnLockDosList(LDF_DEVICES|LDF_READ);
    return ret;
}

int __nocommandline = 1;

int main(void)
{
    LONG res;

    pool = CreatePool(1024, 1024, MEMF_ANY);
    if (!pool)
    {
        PrintFault(ERROR_NO_FREE_STORE, "Automount");
        return RETURN_FAIL;
    }

    NewList((struct List *)&handlerlist);
    res = LoadPrefs("L:automount-config");

    if (res == 0)
    {
        struct BootNode *n;

        ForeachNode(&((struct ExpansionBase *)ExpansionBase)->MountList, n)
        {
            struct DeviceNode *dn = n->bn_DeviceNode;

            D(bug("[Automount] Checking BootNode %b...\n", dn->dn_Name));

            if ((!dn->dn_Task) && (!dn->dn_SegList) && (!dn->dn_Handler) && dn->dn_Startup)
            {
		struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

                D(bug("[Automount] Not mounted\n"));

                if (fssm->fssm_Environ)
                {
                    struct DosEnvec *de = BADDR(fssm->fssm_Environ);

                    if (de)
                    {
                        struct HandlerNode *hn = FindHandler(de->de_DosType);

                        if (hn)
                        {
                            Printf("Mounting %b with %s\n", dn->dn_Name, hn->handler);

                            dn->dn_Handler = CreateBSTR(hn->handler);
                            if (!dn->dn_Handler)
                            {
                                res = ERROR_NO_FREE_STORE;
                                break;
                            }

                            if (!IsMounted(dn))
                            {
                                D(bug("[Automount] Adding DOS entry...\n"));
                                AddDosEntry((struct DosList *)dn);
                            }

                            if (n->bn_Flags & ADNF_STARTPROC)
                            {
                                char *buf;

                                D(bug("[Automount] Starting up...\n"));

                                res = AROS_BSTR_strlen(dn->dn_Name);
                                buf = AllocMem(res + 2, MEMF_ANY);
                                if (!buf)
                                {
                                    res = ERROR_NO_FREE_STORE;
                                    break;
                                }

                                CopyMem(AROS_BSTR_ADDR(dn->dn_Name), buf, res);
                                buf[res++] = ':';
                                buf[res++] = 0;

                                DeviceProc(buf);
                                FreeMem(buf, res);
                            }
                        }
                    }
                }
            }
        }
    }
    else if (res != -1)
    	PrintFault(res, "Automount");

    DeletePool(pool);
    return RETURN_OK;
}
