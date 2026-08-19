/*
 * startup.c - Cocoa GFX and Input initialization for AROS on macOS Apple Silicon
 *
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 */

#include <aros/debug.h>
#include <aros/startup.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <hidd/gfx.h>
#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/mouse.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "cocoa_intern.h"
#include "hostinterface.h"

/* Retrieved at runtime from kernel.resource */
struct HostInterface *HostIFace = NULL;
extern struct OOP_InterfaceDescr CocoaGfx_ifdescr[];
extern struct OOP_InterfaceDescr CocoaBM_ifdescr[];

static struct CocoaGfx_staticdata xsd;
OOP_AttrBase __IMeta;
OOP_AttrBase __IHidd;
OOP_AttrBase __IHidd_BitMap;
OOP_AttrBase __IHidd_Gfx;
OOP_AttrBase __IHidd_Sync;
OOP_AttrBase __IHidd_PixFmt;
OOP_AttrBase __IHidd_ChunkyBM;

int __nocommandline = 1;
int __noinitexitsets = 1;

/* Symbolset handlers for freestanding driver binary */
THIS_PROGRAM_HANDLES_SYMBOLSET(LIBS)
THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(CTORS)
THIS_PROGRAM_HANDLES_SYMBOLSET(DTORS)
THIS_PROGRAM_HANDLES_SYMBOLSET(INIT_ARRAY)
THIS_PROGRAM_HANDLES_SYMBOLSET(FINI_ARRAY)
DEFINESET(LIBS);
DEFINESET(INIT);
DEFINESET(EXIT);
DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT_ARRAY);
DEFINESET(FINI_ARRAY);
void __attribute__((weak)) __register_frame(void *begin) {}
void __attribute__((weak)) __deregister_frame(void *begin) {}

LONG __startup_error;

__startup AROS_PROCH(__startup_entry, argstr, argsize, sysBase)
{
    AROS_PROCFUNC_INIT

    SysBase = sysBase;

    extern int main(void);
    return main();

    AROS_PROCFUNC_EXIT
}

int __startup_error_storage;
int *__startup_error_ptr = &__startup_error_storage;

APTR KernelBase = NULL;

/* Dedicated background task for Cocoa input polling */
static AROS_PROCH(cocoa_input_task, argstr, argsize, sysBase)
{
    AROS_PROCFUNC_INIT
    SysBase = sysBase;
    struct DOSBase *DOSBase = (struct DOSBase *)OpenLibrary("dos.library", 36);
    extern void cocoa_input_poll(struct HostInterface *hif);

    for (;;) {
        cocoa_input_poll(HostIFace);
        if (DOSBase)
            Delay(1);
    }

    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);

    return 0;
    AROS_PROCFUNC_EXIT
}

int main(void)
{
    /* Open required base libraries */
    OOPBase = OpenLibrary("oop.library", 0);
    UtilityBase = (APTR)OpenLibrary("utility.library", 0);
    KernelBase = OpenResource("kernel.resource");

    if (KernelBase && OOPBase && UtilityBase) {
        struct TagItem *tags = KrnGetBootInfo();
        if (tags) {
            struct TagItem *tag = FindTagItem(KRN_HostInterface, tags);
            if (tag) {
                struct HostInterface *hif = (struct HostInterface *)tag->ti_Data;

                if (hif && hif->cocoa_fb_base) {
                    struct GfxBase *GfxBase;

                    HostIFace = hif;
                    xsd.fb_base   = hif->cocoa_fb_base;
                    xsd.fb_width  = hif->cocoa_fb_width;
                    xsd.fb_height = hif->cocoa_fb_height;
                    xsd.fb_pitch  = hif->cocoa_fb_pitch;
                    xsd.iface     = hif;

                    __IMeta        = OOP_ObtainAttrBase(IID_Meta);
                    __IHidd        = OOP_ObtainAttrBase(IID_Hidd);
                    __IHidd_BitMap = OOP_ObtainAttrBase(IID_Hidd_BitMap);
                    __IHidd_Gfx    = OOP_ObtainAttrBase(IID_Hidd_Gfx);
                    __IHidd_Sync   = OOP_ObtainAttrBase(IID_Hidd_Sync);
                    __IHidd_PixFmt = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
                    __IHidd_ChunkyBM = OOP_ObtainAttrBase(IID_Hidd_ChunkyBM);

                    xsd.hiddBitMapAttrBase = __IHidd_BitMap;
                    xsd.hiddGfxAttrBase    = __IHidd_Gfx;
                    xsd.hiddSyncAttrBase   = __IHidd_Sync;
                    xsd.hiddPixFmtAttrBase = __IHidd_PixFmt;

                    if (__IHidd_BitMap && __IHidd_Gfx) {
                        struct TagItem gtags[] = {
                            { aMeta_SuperID,        (IPTR)CLID_Hidd_Gfx },
                            { aMeta_InterfaceDescr, (IPTR)CocoaGfx_ifdescr },
                            { aMeta_ID,             (IPTR)"hidd.gfx.cocoa" },
                            { aMeta_InstSize,       0 },
                            { TAG_DONE, 0 }
                        };
                        struct TagItem btags[] = {
                            { aMeta_SuperID,        (IPTR)CLID_Hidd_ChunkyBM },
                            { aMeta_InterfaceDescr, (IPTR)CocoaBM_ifdescr },
                            { aMeta_InstSize,       sizeof(struct CocoaBMData) },
                            { TAG_DONE, 0 }
                        };

                        xsd.gfxclass = OOP_NewObject(NULL, CLID_HiddMeta, gtags);
                        if (xsd.gfxclass) {
                            xsd.gfxclass->UserData = &xsd;
                            xsd.bmclass = OOP_NewObject(NULL, CLID_HiddMeta, btags);
                            if (xsd.bmclass) {
                                xsd.bmclass->UserData = &xsd;
                                OOP_AddClass(xsd.gfxclass);

                                GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
                                if (GfxBase) {
                                    AddDisplayDriverA(xsd.gfxclass, NULL, NULL);
                                    CloseLibrary((struct Library *)GfxBase);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* Register input drivers */
    {
        extern struct OOP_InterfaceDescr CocoaMouse_ifdescr[];
        extern struct OOP_InterfaceDescr CocoaKbd_ifdescr[];
        extern void cocoa_input_set_objects(OOP_Object *mouse, OOP_Object *kbd);

        struct TagItem mtags[] = {
            { aMeta_SuperID,        (IPTR)CLID_Hidd },
            { aMeta_InterfaceDescr, (IPTR)CocoaMouse_ifdescr },
            { aMeta_ID,             (IPTR)"hidd.mouse.cocoa" },
            { aMeta_InstSize,       16 },
            { TAG_DONE, 0 }
        };
        struct TagItem ktags[] = {
            { aMeta_SuperID,        (IPTR)CLID_Hidd },
            { aMeta_InterfaceDescr, (IPTR)CocoaKbd_ifdescr },
            { aMeta_ID,             (IPTR)"hidd.kbd.cocoa" },
            { aMeta_InstSize,       16 },
            { TAG_DONE, 0 }
        };

        OOP_Class *mouseclass = OOP_NewObject(NULL, CLID_HiddMeta, mtags);
        OOP_Class *kbdclass = OOP_NewObject(NULL, CLID_HiddMeta, ktags);

        if (mouseclass && kbdclass) {
            OOP_AddClass(mouseclass);
            OOP_AddClass(kbdclass);

            OOP_Object *kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
            OOP_Object *ms  = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);

            if (ms && kbd) {
                struct TagItem mstags[] = {{ TAG_DONE, 0 }};
                struct TagItem kbtags[] = {{ TAG_DONE, 0 }};

                OOP_MethodID inputMID = OOP_GetMethodID(IID_Hidd_Input, 0);
                struct pHidd_Input_AddHardwareDriver amsg;
                OOP_Object *msdriver, *kbdriver;

                amsg.mID = inputMID + moHidd_Input_AddHardwareDriver;
                amsg.driverClass = mouseclass;
                amsg.tags = mstags;
                msdriver = (OOP_Object *)OOP_DoMethod(ms, (OOP_Msg)&amsg);

                amsg.driverClass = kbdclass;
                amsg.tags = kbtags;
                kbdriver = (OOP_Object *)OOP_DoMethod(kbd, (OOP_Msg)&amsg);

                if (msdriver || kbdriver) {
                    cocoa_input_set_objects(msdriver, kbdriver);

                    /* Spawn asynchronous background input poller process */
                    struct TagItem ptags[] = {
                        { NP_Entry,     (IPTR)cocoa_input_task },
                        { NP_Name,      (IPTR)"Cocoa Input Poller" },
                        { NP_Priority,  10 },
                        { NP_StackSize, 16384 },
                        { TAG_DONE,     0 }
                    };
                    struct DOSBase *DOSBase = (struct DOSBase *)OpenLibrary("dos.library", 36);
                    if (DOSBase) {
                        CreateNewProc(ptags);
                        CloseLibrary((struct Library *)DOSBase);
                    }
                }
            }
        }
    }

    /* Stay resident and exit cleanly */
    struct Process *me = (struct Process *)FindTask(NULL);
    if (me) {
        if (me->pr_CLI) {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);
            if (cli)
                cli->cli_Module = NULL;
        } else {
            me->pr_SegList = NULL;
        }
    }

    return RETURN_OK;
}
