/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Disk-resident loader for the headless display driver.
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <graphics/driver.h>
#include <oop/oop.h>
#include <workbench/startup.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/oop.h>

#include <stdlib.h>

#include "headlessgfx_hidd.h"

/*
 * AROSMonDrvs executes what it finds in DEVS:Monitors, and that is what
 * this is. Loading the driver library only sets its classes up; the
 * driver is registered with graphics.library here, so that the user's
 * depth configuration - the MAXDEPTH and FORCEDEPTH tooltypes of this
 * program's icon - can be handed to the driver object when it is
 * created.
 */

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

static OOP_AttrBase HiddGfxHeadlessAttrBase;

int main(void)
{
    struct Library *HeadlessGfxBase;
    struct DiskObject *icon;
    OOP_Class *cl;
    BPTR olddir = BNULL;
    STRPTR myname;
    ULONG maxdepth = 0, forcedepth = 0;
    ULONG width = 0, height = 0, nominaldepth = 0;
    ULONG err;
    struct TagItem attrs[6];
    ULONG nattrs = 0;
    struct TagItem ddtags[] =
    {
        { DDRV_BootMode, TRUE },
        { TAG_DONE,      0    }
    };

    /*
     * Already present (a kickstart-resident copy registered itself, or
     * we ran twice)? Then there is nothing left for us to do.
     */
    if (FindName(&SysBase->LibList, "headlessgfx.hidd"))
        return RETURN_OK;

    if (WBenchMsg)
    {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
        myname = WBenchMsg->sm_ArgList[0].wa_Name;
    }
    else
    {
        struct Process *me = (struct Process *)FindTask(NULL);

        if (me->pr_CLI)
        {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);

            myname = AROS_BSTR_ADDR(cli->cli_CommandName);
        }
        else
            myname = me->pr_Task.tc_Node.ln_Name;
    }
    D(bug("[HeadlessGfx:Disk] command name: %s\n", myname));

    icon = GetDiskObject(myname);
    if (icon)
    {
        STRPTR str;

        str = FindToolType(icon->do_ToolTypes, "MAXDEPTH");
        if (str)
            maxdepth = atoi(str);

        str = FindToolType(icon->do_ToolTypes, "FORCEDEPTH");
        if (str)
            forcedepth = atoi(str);

        str = FindToolType(icon->do_ToolTypes, "WIDTH");
        if (str)
            width = atoi(str);

        str = FindToolType(icon->do_ToolTypes, "HEIGHT");
        if (str)
            height = atoi(str);

        str = FindToolType(icon->do_ToolTypes, "DEPTH");
        if (str)
            nominaldepth = atoi(str);

        FreeDiskObject(icon);
    }
    if (olddir)
        CurrentDir(olddir);

    D(bug("[HeadlessGfx:Disk] MAXDEPTH %u, FORCEDEPTH %u, WIDTH %u, HEIGHT %u, DEPTH %u\n",
          maxdepth, forcedepth, width, height, nominaldepth));

    HeadlessGfxBase = OpenLibrary("headlessgfx.hidd", 0);
    if (HeadlessGfxBase == NULL)
        HeadlessGfxBase = OpenLibrary("DEVS:Drivers/headlessgfx.hidd", 0);

    if (HeadlessGfxBase == NULL)
    {
        D(bug("[HeadlessGfx:Disk] headlessgfx.hidd would not open\n"));
        return RETURN_FAIL;
    }

    D(bug("[HeadlessGfx:Disk] driver loaded @ 0x%p\n", HeadlessGfxBase));

    cl = OOP_FindClass(CLID_Hidd_Gfx_Headless);
    HiddGfxHeadlessAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gfx_Headless);
    if (cl == NULL || HiddGfxHeadlessAttrBase == 0)
    {
        if (HiddGfxHeadlessAttrBase)
            OOP_ReleaseAttrBase(IID_Hidd_Gfx_Headless);
        CloseLibrary(HeadlessGfxBase);
        return RETURN_FAIL;
    }

    if (maxdepth)
    {
        attrs[nattrs].ti_Tag  = aHidd_Gfx_Headless_MaxDepth;
        attrs[nattrs].ti_Data = maxdepth;
        nattrs++;
    }
    if (forcedepth)
    {
        attrs[nattrs].ti_Tag  = aHidd_Gfx_Headless_FixedDepth;
        attrs[nattrs].ti_Data = forcedepth;
        nattrs++;
    }
    if (width)
    {
        attrs[nattrs].ti_Tag  = aHidd_Gfx_Headless_Width;
        attrs[nattrs].ti_Data = width;
        nattrs++;
    }
    if (height)
    {
        attrs[nattrs].ti_Tag  = aHidd_Gfx_Headless_Height;
        attrs[nattrs].ti_Data = height;
        nattrs++;
    }
    if (nominaldepth)
    {
        attrs[nattrs].ti_Tag  = aHidd_Gfx_Headless_NominalDepth;
        attrs[nattrs].ti_Data = nominaldepth;
        nattrs++;
    }
    attrs[nattrs].ti_Tag  = TAG_DONE;
    attrs[nattrs].ti_Data = 0;

    err = AddDisplayDriverA(cl, attrs, ddtags);
    D(bug("[HeadlessGfx:Disk] AddDisplayDriverA() result: %u\n", err));

    OOP_ReleaseAttrBase(IID_Hidd_Gfx_Headless);

    if (err)
    {
        CloseLibrary(HeadlessGfxBase);
        return RETURN_FAIL;
    }

    /*
     * Left open on purpose - closing the last reference would expunge
     * the driver that has just been registered.
     */
    return RETURN_OK;
}
