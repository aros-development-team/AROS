/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define DEBUG 0

#include <aros/debug.h>

#include <exec/types.h>
#include <oop/oop.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <devices/timer.h>

#include <hidd/hidd.h>
#include <usb/usb.h>
#include <usb/usb_core.h>

#include <proto/oop.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "usb.h"

/*
 * Alloc a unique 7-bit address for a device
 */
uint8_t allocBitmap(uint32_t *bmp)
{
    uint8_t addr;

    for (addr = 0; addr < BITMAP_SIZE; addr++)
    {
        int byte = addr / 32;
        int bit = 31 - (addr % 32);

        if (!(bmp[byte] & (1 << bit))) {
            bmp[byte] |= 1 << bit;
            break;
        }
    }

    if (addr == BITMAP_SIZE)
        addr = 0xff;

    return addr;
}

/*
 * Make a 7-bit address free for use
 */
void freeBitmap(uint32_t *bmp, uint8_t addr)
{
    uint8_t byte = addr / 32;
    uint8_t bit = 31 - (addr % 32);

    if (addr < BITMAP_SIZE)
    {
        bmp[byte] &= ~(1 << bit);
    }
}

/*
 * AllocAbs the specified address
 */
void setBitmap(uint32_t *bmp, uint8_t addr)
{
    uint8_t byte = addr / 32;
    uint8_t bit = 31 - (addr % 32);

    if (addr < BITMAP_SIZE)
    {
        bmp[byte] |= 1 << bit;
    }
}

static void ScanDirectory(struct Library *DOSBase, struct usb_staticdata *sd, STRPTR dir)
{
    struct AnchorPath   ap;
    uint8_t               match[2048];
    int32_t             err;

    snprintf(match, sizeof(match)-1, "%s/#?.hidd", dir);

    D(bug("[USB] match = \"%s\"\n", match));

    memset(&ap, 0, sizeof(ap));

    err = MatchFirst(match, &ap);
    while ((err == 0))
    {
        if (ap.ap_Info.fib_EntryType < 0)
        {
            struct usb_ExtClass *ec = NULL;
            int found = 0;

            snprintf(match, sizeof(match)-1, "%s/%s", dir, ap.ap_Info.fib_FileName);

            ForeachNode(&sd->extClassList, ec)
            {
                if (!strcmp(ap.ap_Info.fib_FileName, ec->ec_ShortName))
                {
                    found = 1;
                    break;
                }
            }

            if (!found)
            {

                ec = AllocVecPooled(sd->MemPool, sizeof(struct usb_ExtClass));

                D(bug("[USB] external class \"%s\" not yet in the list. Adding it\n", match));

                if (ec)
                {
//                    ec->ec_LibBase = OpenLibrary(match, 0);
//                    if (!ec->ec_LibBase)
//                    {
//                        FreeVecPooled(sd->MemPool, ec);
//                    }
//                    else
                    {
                        ec->ec_Node.ln_Name = AllocVecPooled(sd->MemPool, strlen(match)+1);
                        CopyMem(match, ec->ec_Node.ln_Name, strlen(match)+1);
                        ec->ec_ShortName = AllocVecPooled(sd->MemPool, strlen(ap.ap_Info.fib_FileName)+1);
                        CopyMem(ap.ap_Info.fib_FileName, (char *)ec->ec_ShortName, strlen(ap.ap_Info.fib_FileName)+1);
                        AddTail(&sd->extClassList, &ec->ec_Node);
                    }
                }
            }
        }
        err = MatchNext(&ap);
    }
    MatchEnd(&ap);
}

void UpdatePaths(struct Library *DOSBase, struct usb_staticdata *sd)
{
    struct DosList *dl = LockDosList(LDF_READ | LDF_ASSIGNS);

    if (dl)
    {
        if ((dl = FindDosEntry(dl, "USBCLASSES", LDF_ASSIGNS)) != NULL)
        {
            struct AssignList *nextAssign;
            uint8_t dirName[2048];

            if (NameFromLock(dl->dol_Lock, dirName, sizeof(dirName)))
            {
                D(bug("[USB] Scanning '%s'\n", dirName));

                ScanDirectory(DOSBase, sd, dirName);
            }

            nextAssign = dl->dol_misc.dol_assign.dol_List;
            while (nextAssign != NULL)
            {
                if (NameFromLock(nextAssign->al_Lock, dirName, sizeof(dirName)))
                {
                    D(bug("[USB] Scanning '%s'\n", dirName));

                    ScanDirectory(DOSBase, sd, dirName);
                }

                nextAssign = nextAssign->al_Next;
            }
        }

        UnLockDosList(LDF_READ | LDF_ASSIGNS);
    }
}

static void CleanupProcess()
{
}

void usb_process()
{
    struct usb_staticdata *sd = (struct usb_staticdata *)(FindTask(NULL)->tc_UserData);
    struct Process *usbProcess = (struct Process *)FindTask(NULL);
    struct usbEvent *ev = NULL;
    struct MsgPort *port;
    struct timerequest *tr;

    struct Library *DOSBase = OpenLibrary("dos.library", 0);

    port = CreateMsgPort();
    tr = CreateIORequest(port, sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0);

    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_sec = 10;
    tr->tr_time.tv_usec = 0;
    SendIO((struct IORequest *)tr);

    D(bug("[USB Process] Hello. Task @ %p, signals = %08x\n", FindTask(NULL), FindTask(NULL)->tc_SigAlloc));

    UpdatePaths(DOSBase, sd);

    for(;;)
    {
        Wait((1 << usbProcess->pr_MsgPort.mp_SigBit) |
             (1 << port->mp_SigBit));

        if (GetMsg(port))
        {
            UpdatePaths(DOSBase, sd);

            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_sec = 10;
            tr->tr_time.tv_usec = 0;
            SendIO((struct IORequest *)tr);
        }

        while ((ev = (struct usbEvent *)GetMsg(&usbProcess->pr_MsgPort)) != NULL)
        {
            switch (ev->ev_Type)
            {
                case evt_Startup:
                    D(bug("[USB Process] Startup MSG\n"));
                    ReplyMsg(&ev->ev_Message);
                    break;

                case evt_Method:
                    D(bug("[USB Process] Sync method call\n"));
                    ev->ev_RetVal = OOP_DoMethod(ev->ev_Target, (OOP_Msg)&ev->ev_Event);
                    ReplyMsg(&ev->ev_Message);
                    break;

                case evt_AsyncMethod:
                    D(bug("[USB Process] Async method call\n"));
                    OOP_DoMethod(ev->ev_Target, (OOP_Msg)&ev->ev_Event);
                    FreeVecPooled(sd->MemPool, ev);
                    break;

                case evt_Cleanup:
                    D(bug("[USB Process] Cleanup MSG\n"));
                    CleanupProcess();
                    ReplyMsg(&ev->ev_Message);
                    CloseLibrary(DOSBase);
                    return;

                default:
                    break;
            }
        }
    }
}
