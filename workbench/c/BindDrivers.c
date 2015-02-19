/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BindDrivers CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        BindDrivers

    SYNOPSIS

        DEVICES/S,DRIVERS/S,DIR/K/A

    LOCATION

        C:

    FUNCTION

        For all device drivers with a .info file in SYS:Expansion, load
        the device driver via Exec/InitResident() if its PRODUCT=
        tooltype matches a device that is in the system, and not yet
        configured.

    INPUTS

        DEVICES           -- List all devices, and their bindings

        DRIVERS           -- List all drivers, and their supported products

        DIR <directory>   -- Directory to search, instead of SYS:Expansion/

    RESULT

    NOTES


    EXAMPLE

        C:BindDrivers

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    06.01.2012  Jason S. McMullan - Implemented

******************************************************************************/

#define __NOLIBBASE__

#include <aros/debug.h>

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>

#include <exec/resident.h>

#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/expansion.h>

#if DEBUG
#define SH_GLOBAL_SYSBASE 1     /* for kprintf() */
#endif
#include <aros/shcommands.h>

#define PROD_ANY        ~0

struct BindDriverNode {
    struct Node bd_Node;
    UBYTE      *bd_ProductString;
    struct DiskObject *bd_Icon;

    /* Must be at the end of this extendable structure.. */
    ULONG       bd_Products;
    struct {
        UWORD mfg;
        UWORD prod;
    } bd_Product[0];
};

static LONG BindDriverAdd(struct List *drivers, CONST_STRPTR name, APTR IconBase, APTR SysBase)
{
    struct BindDriverNode *bd;
    struct DiskObject *icon;
    LONG err = 0;
    UBYTE *product, *cp;

    icon = GetDiskObject(name);
    if (icon == NULL)
        return err;

    if ((product = FindToolType(icon->do_ToolTypes, "PRODUCT"))) {
        int products = 1;
        for (cp = product; *cp; cp++) {
            if (!(isdigit(*cp) || *cp == '|' || *cp == '/'))
                break;
            if (*cp == '|')
                products++;
        }
        if (*cp == 0) {
            /* String had only valid characters */
            if ((bd = AllocVec(sizeof(*bd) + sizeof(bd->bd_Product[0])*products + strlen(name) + 1, MEMF_ANY | MEMF_CLEAR))) {
                int i;

                bd->bd_ProductString = product;
                bd->bd_Node.ln_Name = (APTR)&bd->bd_Product[products];
                bd->bd_Icon = icon;
                D(bug("%s: bd=%lx, icon=%lx\n", __func__, bd, icon));
                strcpy(bd->bd_Node.ln_Name, name);
                for (i = 0, cp = product; *cp; i++) {
                    unsigned long val;
                    char *next;

                    val = strtoul(cp, &next, 10);
                    if (next == (char *)cp)
                        break;

                    bd->bd_Product[i].mfg = val;
                    if (*next == '/') {
                        cp = next+1;
                        val = strtoul(cp, &next, 10);
                        if (next == (char *)cp)
                            break;
                        bd->bd_Product[i].prod = val;
                    } else {
                        bd->bd_Product[i].prod = PROD_ANY;
                    }

                    if (*next && *next != '|')
                        break;

                    bd->bd_Products++;

                    if (*next)
                        cp = next+1;
                    else
                        break;
                }

                if (bd->bd_Products) {
                    AddTail(drivers, &bd->bd_Node);
                    err = 0;
                } else {
                    FreeDiskObject(bd->bd_Icon);
                    FreeVec(bd);
                    err = RETURN_FAIL;
                }
            }
        }
    } else {
        D(bug("%s: \tNo PRODUCT= tooltype\n", __func__));
    }

    return err;
}

static struct Resident *SearchResident(BPTR seglist)
{
    const int ressize = offsetof(struct Resident, rt_Init) + sizeof(APTR);

    D(bug("%lx: Search for resident...\n", BADDR(seglist)));
    while (seglist) {
        APTR addr = (APTR)((IPTR)BADDR(seglist) - sizeof(ULONG));
        ULONG size = *(ULONG *)addr;

        for (addr += sizeof(BPTR) + sizeof(ULONG),
             size -= sizeof(BPTR) + sizeof(ULONG);
             size >= ressize;
             size -= 2, addr += 2) {
            struct Resident *res = (struct Resident *)addr;

            if (res->rt_MatchWord == RTC_MATCHWORD &&
                res->rt_MatchTag  == res) {
                D(bug("%lx: Resident found at %lx\n", BADDR(seglist), res));
                return res;
            }
        }
        
        seglist = *(BPTR *)BADDR(seglist);
    }

    D(bug("%lx: No resident\n", BADDR(seglist)));
    return NULL;
}

static LONG BindDriver(STRPTR name, UWORD mfg, UBYTE prod, UBYTE *prodstr, UBYTE **tooltypes, APTR ExpansionBase, APTR DOSBase, APTR SysBase)
{
    BPTR seglist;
    LONG err = RETURN_OK;

    struct CurrentBinding cb;
    cb.cb_ConfigDev = NULL;
    cb.cb_FileName = name;
    cb.cb_ProductString = prodstr;
    cb.cb_ToolTypes = tooltypes;

    while ((cb.cb_ConfigDev = FindConfigDev(cb.cb_ConfigDev, mfg, prod))) {

        if (cb.cb_ConfigDev->cd_Flags & CDF_SHUTUP)
            continue;

        if (!(cb.cb_ConfigDev->cd_Flags & CDF_CONFIGME))
            continue;

        if ((seglist = LoadSeg(name)) != BNULL) {
            struct Resident *res;

            if ((res = SearchResident(seglist))) {
                D(bug("Binding=%lx, name=%s, res=%lx\n", &cb, name, res));
                ObtainConfigBinding();
                SetCurrentBinding(&cb, sizeof(cb));
                D(bug("Calling InitResident via %lx\n", (IPTR)SysBase - 6 * 17));
                if (InitResident(res, seglist) == NULL) {
                    D(bug("\tfailed\n"));
                    UnLoadSeg(seglist);
                } else {
                    D(bug("\tbound\n"));
                }
                ReleaseConfigBinding();
            } else {
                /* No resident? Then never loadable */
                err = RETURN_FAIL;
                D(bug("No resident for %s\n", name));
                UnLoadSeg(seglist);
            }
        } else {
            /* Can't load the file? Then don't try again */
            err = RETURN_FAIL;
        }

        if (err != RETURN_OK) {
            break;
        }
    }

    return err;
}


AROS_SH3(BindDrivers, 41.1,
AROS_SHA(BOOL, ,DRIVERS,/S, FALSE),
AROS_SHA(BOOL, ,DEVICES,/S, FALSE),
AROS_SHA(STRPTR, ,DIR,/K, "SYS:Expansion"))
{

    AROS_SHCOMMAND_INIT

    struct ExAllControl *eac;
    BPTR lock, olddir;
    struct List drivers;
    struct BindDriverNode *node, *tmp;
    struct Library *ExpansionBase;
    struct Library *IconBase;
    LONG error;

    if (!(ExpansionBase = OpenLibrary("expansion.library", 33)))
        return RETURN_FAIL;

    if (!(IconBase = OpenLibrary("icon.library", 36))) {
        CloseLibrary(ExpansionBase);
        return RETURN_FAIL;
    }

    /* Just dump what devices we have */
    if (SHArg(DEVICES)) {
        struct ConfigDev *cdev = NULL;

        ObtainConfigBinding();
        while ((cdev = FindConfigDev(cdev, -1, -1))) {
            struct Node *node = cdev->cd_Driver;
            Printf("%5ld/%-3ld %08lx-%08lx %s\n",
                    cdev->cd_Rom.er_Manufacturer,
                    cdev->cd_Rom.er_Product,
                    (ULONG)(IPTR)cdev->cd_BoardAddr,
                    (ULONG)(IPTR)cdev->cd_BoardAddr+cdev->cd_BoardSize-1,
                    (cdev->cd_Flags & CDF_CONFIGME) ?
                     "(unbound)" : node->ln_Name);
        }
        ReleaseConfigBinding();
        CloseLibrary(IconBase);
        CloseLibrary(ExpansionBase);
        return RETURN_OK;
    }

    NEWLIST(&drivers);

    lock = Lock(SHArg(DIR), SHARED_LOCK);
    if (lock == BNULL) {
        error = IoErr();
        Printf("BindDrivers: Can't open %s\n", SHArg(DIR));
        CloseLibrary(IconBase);
        CloseLibrary(ExpansionBase);
        SetIoErr(error);
        return RETURN_FAIL;
    }

    olddir = CurrentDir(lock);

    if ((eac = AllocDosObject(DOS_EXALLCONTROL,NULL))) {
        BOOL more;
        UBYTE eadarr[256];

        do {
            more = ExAll(lock, (struct ExAllData *)&eadarr[0], sizeof(eadarr), ED_NAME, eac);
            if (!more && (IoErr() != ERROR_NO_MORE_ENTRIES))
                break;

            if (eac->eac_Entries) {
                struct ExAllData *ead;
                
                for (ead = (APTR)&eadarr[0]; ead; ead=ead->ed_Next)
                    BindDriverAdd(&drivers, ead->ed_Name, IconBase, SysBase);
            }
        } while (more);
    }

    ForeachNodeSafe(&drivers, node, tmp) {
        int i;
        for (i = 0; i < node->bd_Products; i++) {
            /* If SHArg(DRIVERS) is true, just list the drivers
             */
            if (SHArg(DRIVERS)) {
                Printf("%5ld/%-3ld %s\n", (ULONG)node->bd_Product[i].mfg, (ULONG)node->bd_Product[i].prod, node->bd_Node.ln_Name);
            } else {
                LONG err;
                err = BindDriver(node->bd_Node.ln_Name, node->bd_Product[i].mfg, node->bd_Product[i].prod, node->bd_ProductString, node->bd_Icon->do_ToolTypes, ExpansionBase, DOSBase, SysBase);
                if (err != RETURN_OK)
                    break;
            }
        }
        Remove(&node->bd_Node);
        FreeDiskObject(node->bd_Icon);
        FreeVec(node);
    }

    CurrentDir(olddir);
    UnLock(lock);

    CloseLibrary(IconBase);
    CloseLibrary(ExpansionBase);
    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
