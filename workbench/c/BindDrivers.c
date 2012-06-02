/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>
/******************************************************************************


    NAME

        BindDrivers

    SYNOPSIS

        DEVICES/S,DRIVERS/S

    LOCATION

        C:

    FUNCTION

        For all device drivers with a .info file in SYS:Expansion, load
        the device driver via Exec/InitResident() if its PRODUCT=
        tooltype matches a device that is in the system, and not yet
        configured.

    INPUTS

        DEVICES           --  List all devices, and their bindings

        DRIVERS           --  List all drivers, and their supported products

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

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>

#include <exec/resident.h>

#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/expansion.h>

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

static LONG BindDriverAdd(struct Library *IconBase, struct List *drivers, CONST_STRPTR name)
{
    struct BindDriverNode *bd;
    struct DiskObject *icon;
    LONG err = 0;
    UBYTE *product, *cp;

    icon = GetIconTags(name, ICONGETA_FailIfUnavailable, TRUE,
                             ICONGETA_GetPaletteMappedIcon, FALSE,
                             ICONGETA_RemapIcon, FALSE,
                             ICONGETA_GenerateImageMasks, FALSE,
                             ICONGETA_Screen, NULL,
                             ICONA_ErrorCode, &err);
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
                D(bug("%s: bd=%p, icon=%p\n", __func__, bd, icon));
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

    while (seglist) {
        APTR addr = (APTR)((IPTR)BADDR(seglist) - sizeof(ULONG));
        ULONG size = *(ULONG *)addr;

        for (addr += sizeof(BPTR) + sizeof(ULONG),
             size -= sizeof(BPTR) + sizeof(ULONG);
             size >= ressize;
             size -= 2, addr += 2) {
            struct Resident *res = (struct Resident *)addr;

            if (res->rt_MatchWord == RTC_MATCHWORD &&
                res->rt_MatchTag  == res)
                return res;
        }
        
        seglist = *(BPTR *)BADDR(seglist);
    }

    return NULL;
}

static LONG BindDriver(struct Library *DOSBase, STRPTR name, UWORD mfg, UBYTE prod, UBYTE *prodstr, UBYTE **tooltypes)
{
    BPTR seglist;
    LONG err = RETURN_OK;
    struct Library *ExpansionBase;

    struct CurrentBinding cb;
    cb.cb_ConfigDev = NULL;
    cb.cb_FileName = name;
    cb.cb_ProductString = prodstr;
    cb.cb_ToolTypes = tooltypes;

    if ((ExpansionBase = TaggedOpenLibrary(TAGGEDOPEN_EXPANSION))) {
        while ((cb.cb_ConfigDev = FindConfigDev(cb.cb_ConfigDev, mfg, prod))) {

            if (cb.cb_ConfigDev->cd_Flags & CDF_SHUTUP)
                continue;

            if (!(cb.cb_ConfigDev->cd_Flags & CDF_CONFIGME))
                continue;

            if ((seglist = LoadSeg(name)) != BNULL) {
                struct Resident *res;

                if ((res = SearchResident(seglist))) {
                    ObtainConfigBinding();
                    SetCurrentBinding(&cb, sizeof(cb));
                    Forbid();
                    InitResident(res, seglist);
                    Permit();
                    ReleaseConfigBinding();
                } else {
                    /* No resident? Then never loadable */
                    err = RETURN_FAIL;
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
        CloseLibrary(ExpansionBase);
    }

    return err;
}

AROS_SH2(BindDrivers, 41.1,
AROS_SHA(BOOL, ,DRIVERS,/S, FALSE),
AROS_SHA(BOOL, ,DEVICES,/S, FALSE))
{

    AROS_SHCOMMAND_INIT

    struct ExAllControl *eac;
    BPTR lock, olddir;
    struct List drivers;
    struct Library *IconBase;
    struct BindDriverNode *node, *tmp;

    /* Just dump what devices we have */
    if (SHArg(DEVICES)) {
        struct Library *ExpansionBase;

        if ((ExpansionBase = TaggedOpenLibrary(TAGGEDOPEN_EXPANSION))) {
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
            CloseLibrary(ExpansionBase);
            return RETURN_OK;
        }

        return RETURN_FAIL;
    }

    if (!(IconBase = TaggedOpenLibrary(TAGGEDOPEN_ICON))) {
        Printf("BindDrivers: Can't open icon.library\n");
        return RETURN_FAIL;
    }

    NEWLIST(&drivers);

    lock = Lock("SYS:Expansion", SHARED_LOCK);
    if (lock == BNULL) {
        Printf("BindDrivers: Can't open SYS:Expansion\n");
        return IoErr();
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
                    BindDriverAdd(IconBase, &drivers, ead->ed_Name);
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
                err = BindDriver((struct Library *)DOSBase, node->bd_Node.ln_Name, node->bd_Product[i].mfg, node->bd_Product[i].prod, node->bd_ProductString, node->bd_Icon->do_ToolTypes);
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

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
