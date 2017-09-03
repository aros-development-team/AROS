/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* A very hacky testcase which dumps lots of graphics subsystem internal things.
   Please do not rely on the information you obtain using this program! Remember
   that it some of things it will print are really private and internal! Do not
   use techniques used by this program in common user software! */

/* Define this if you have no CGX SDK for some reason. You'll miss one bit of information then.
#define NO_CGX_API */

#ifdef __amigaos4__
#define __USE_INLINE__
#endif

#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>
#include <proto/dos.h>
#define __USE_BASETYPE__ // For __amigaos4__
#include <proto/exec.h>
#include <proto/graphics.h>
#undef __USE_BASETYPE__

#include <stdio.h>
#include <string.h>

#ifndef NO_CGX_API
#include <proto/cybergraphics.h>
#endif

#ifndef __AROS__
#define IPTR ULONG
#endif

struct myargs
{
    IPTR nospecs;
    IPTR nomodes;
    IPTR allspecs;
    IPTR displaydb;
};

/* This stuff is made static in order to reduce stack usage */
static struct myargs args = {FALSE, FALSE, FALSE, FALSE};
static struct NameInfo ni;
static struct DisplayInfo di;
static struct DimensionInfo dims;
static struct MonitorInfo mon;

int __nocommandline = 1;

static void PrintList(struct List *l)
{
    printf("    lh_Head     %p\n", l->lh_Head);
    printf("    lh_Tail     %p\n", l->lh_Tail);
    printf("    lh_TailPred %p\n", l->lh_TailPred);
    printf("    lh_Type     %u\n", l->lh_Type);
}

static void PrintName(char *name, struct ExtendedNode *n)
{
    printf("%s %p", name, n);
    if (n)
        printf(" %s", n->xln_Name);
    printf("\n");
}

static void PrintNode(char *name, struct ExtendedNode *n)
{
    char *nodename = "<no name>";

    if (n->xln_Name)
        nodename = n->xln_Name;
    printf("%s %p %s\n", name, n, nodename);
    printf("  xln_Succ      %p\n", n->xln_Succ);
    printf("  xln_Pred      %p\n", n->xln_Pred);
    printf("  xln_Type      %d\n", n->xln_Type);
    printf("  xln_Pri       %d\n", n->xln_Pri);
    printf("  xln_Subsystem %d\n", n->xln_Subsystem);
    printf("  xln_Subtype   %d\n", n->xln_Subtype);
    printf("  xln_Library   %p\n", (void *)n->xln_Library);
    printf("  xln_Init      %p\n", n->xln_Init);
}

static inline void PrintPoint(char *name, Point *p)
{
    printf("%s (%u, %u)\n", name, p->x, p->y);
}

static inline void PrintRectangle(char *name, struct Rectangle *r)
{
    printf("%s (%d, %d) - (%d, %d)\n", name, r->MinX, r->MinY, r->MaxX, r->MaxY);
}

static inline void PrintASI(char *name, struct AnalogSignalInterval *sig)
{
    printf("%s Start %u Stop %u\n", name, sig->asi_Start, sig->asi_Stop);
}

static void PrintMonitorSpec(struct MonitorSpec *mspc)
{
    PrintNode("MonitorSpec", &mspc->ms_Node);
    printf        ("  ms_Flags               0x%04X\n",  mspc->ms_Flags);
    printf        ("  ratioh                 %d\n"    ,  (int)mspc->ratioh);
    printf        ("  ratiov                 %d\n"    ,  (int)mspc->ratiov);
    printf        ("  total_rows             %u\n"    ,  mspc->total_rows);
    printf        ("  total_colorclocks      %u\n"    ,  mspc->total_colorclocks);
    printf        ("  DeniseMaxDisplayColumn %u\n"    ,  mspc->DeniseMaxDisplayColumn);
    printf        ("  BeamCon0               0x%04X\n",  mspc->BeamCon0);
    printf        ("  min_row                %u\n"    ,  mspc->min_row);
    printf        ("  ms_Special             %p\n"    ,  mspc->ms_Special);
    printf        ("  ms_OpenCount           %u\n"    ,  mspc->ms_OpenCount);
    printf        ("  ms_transform           %p\n"    ,  mspc->ms_transform);
    printf        ("  ms_translate           %p\n"    ,  mspc->ms_translate);
    printf        ("  ms_scale               %p\n"    ,  mspc->ms_scale);
    printf        ("  ms_xoffset             %u\n"    ,  mspc->ms_xoffset);
    printf        ("  ms_yoffset             %u\n"    ,  mspc->ms_yoffset);
    PrintRectangle("  ms_LegalView          "         , &mspc->ms_LegalView);
    printf        ("  ms_maxoscan            %p\n"    ,  mspc->ms_maxoscan);
    printf        ("  ms_videoscan           %p\n"    ,  mspc->ms_videoscan);
    printf        ("  DeniseMinDisplayColumn %u\n"    ,  mspc->DeniseMinDisplayColumn);
    printf        ("  DisplayCompatible      0x%08X\n",  (unsigned)mspc->DisplayCompatible);
    printf        ("  DisplayInfoDataBase    %p\n"    , &mspc->DisplayInfoDataBase);
    PrintList(&mspc->DisplayInfoDataBase);
    printf        ("  ms_MrgCop              %p\n"    ,  mspc->ms_MrgCop);
    printf        ("  ms_LoadView            %p\n"    ,  mspc->ms_LoadView);
    if (GfxBase->LibNode.lib_Version > 38)
        printf    ("  ms_KillView            %p\n"    ,  mspc->ms_KillView);

    if (mspc->ms_Special) {
	PrintNode("SpecialMonitor", &mspc->ms_Special->spm_Node);
        printf   ("  spm_Flags  0x%04X\n",  mspc->ms_Special->spm_Flags);
        printf   ("  do_monitor %p\n"    ,  mspc->ms_Special->do_monitor);
        printf   ("  reserved1  %p\n"    ,  mspc->ms_Special->reserved1);
	printf   ("  reserved2  %p\n"    ,  mspc->ms_Special->reserved2);
	printf   ("  reserved3  %p\n"    ,  mspc->ms_Special->reserved3);
	PrintASI ("  hblank    "         , &mspc->ms_Special->hblank);
	PrintASI ("  vblank    "         , &mspc->ms_Special->vblank);
	PrintASI ("  hsync     "         , &mspc->ms_Special->hsync);
	PrintASI ("  vsync     "         , &mspc->ms_Special->vsync);
    }
    
    if (args.displaydb) {
        /* We don't use DisplayInfoDataBaseSemaphore here because it may be
	   not initialized in fake MonitorSpecs.
	   What is done here is actually hack. Noone will ever need in
	   a common software. I examined many systems and these lists
	   were either empty or not initialized at all. However
	   there can be a theoretical possibility that something uses them. */
        struct Node *n = mspc->DisplayInfoDataBase.lh_Head;

	if (n && mspc->DisplayInfoDataBase.lh_TailPred) {
	    printf("DisplayInfoDataBase\n");
	    for (; n->ln_Succ; n = n->ln_Succ) {
	        printf("  Node %p %s\n", n, n->ln_Name);
	        printf("    ln_Type %d\n", n->ln_Type);
                printf("    ln_Pri  %d\n", n->ln_Pri);
	    }
	}
    }
}

int main(void)
{
    struct Library *CyberGfxBase;
#ifdef __amigaos4__
    struct CyberGfxIFace *ICyberGfx;
#endif
    struct Library *P96Base;
    struct RDArgs *rda;
    struct MonitorSpec *mspc;
    ULONG modeid = INVALID_ID;

    rda = ReadArgs("NOSPECS/S,NOMODES/S,FORCESPECS/S,DISPLAYDB/S", (IPTR *)&args, NULL);
    if (!rda) {
        printf("You may supply the following switches:\n"
	       "NOSPECS    - Do not list MonitorSpecs in GfxBase\n"
	       "NOMODES    - Do not list Mode IDs\n"
	       "FORCESPECS - When listing Mode IDs, list MonitorSpec contents even if this MonitorSpec\n"
	       "             is found in the GfxBase list. Use this in conjunction with NOSPECS to get\n"
	       "             a valid information\n"
	       "DISPLAYDB  - attempt to list internal nodes of DisplayInfoDataBase lists inside MonitorSpecs.\n"
	       "             May crash or output garbage since it is not proven yet that these nodes actually\n"
	       "             have names.\n");
	return RETURN_FAIL;
    }

    CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    P96Base = OpenLibrary("Picasso96API.library", 0);

    printf("******** System information ********\n\n");
    printf    ("graphics.library      v%u.%u\n", GfxBase->LibNode.lib_Version, GfxBase->LibNode.lib_Revision);
    if (CyberGfxBase)
    {
        printf("cybergraphics.library v%u.%u\n", CyberGfxBase->lib_Version, CyberGfxBase->lib_Revision);
#ifdef __amigaos4__
	ICyberGfx = (struct CyberGfxIFace *)GetInterface((struct Library *)CyberGfxBase, "main", 1, NULL);
#endif
    }
    if (P96Base)
	printf("Picasso96API.library  v%u.%u\n", P96Base->lib_Version, P96Base->lib_Revision);
    printf("\n");

    printf("GfxBase %p\n", GfxBase);
    printf   ("  DisplayFlags    0x%04X\n",  GfxBase->DisplayFlags);
    printf   ("  ChipRevBits0    0x%02X\n",  GfxBase->ChipRevBits0);
    printf   ("  MemType         0x%02X\n",  GfxBase->MemType);
    printf   ("  monitor_id      0x%04X\n",  GfxBase->monitor_id);
    PrintName("  current_monitor"         , &GfxBase->current_monitor->ms_Node);
    PrintName("  default_monitor"         , &GfxBase->default_monitor->ms_Node);
    printf   ("  WantChips       0x%02X\n",  GfxBase->WantChips);
    printf   ("  BoardMemType    0x%02X\n",  GfxBase->BoardMemType);
    printf   ("  Bugs            0x%02X\n",  GfxBase->Bugs);
    PrintName("  natural_monitor"         , &GfxBase->natural_monitor->ms_Node);
    printf   ("  GfxFlags        0x%04X\n",  GfxBase->GfxFlags);
    printf   ("\n");

    printf("CyberGfxBase %p\n", CyberGfxBase);
    printf("P96Base      %p\n", P96Base);

    if (!args.nospecs) {
            printf("*********** MonitorSpecs ***********\n\n");

	  /* It's a good idea to lock this semaphore. It seems to be present in all OSes
            (at least in AmigaOS v3, MorphOS and AROS)
	    However at least on AmigaOS v3 we can't call NextDisplayInfo() and such
	    while the lock is held. */
            ObtainSemaphoreShared(GfxBase->MonitorListSemaphore);
            for (mspc = (struct MonitorSpec *)GfxBase->MonitorList.lh_Head; mspc->ms_Node.xln_Succ; mspc = (struct MonitorSpec *)mspc->ms_Node.xln_Succ) {
                PrintMonitorSpec(mspc);
	        printf("\n");
            }
	    ReleaseSemaphore(GfxBase->MonitorListSemaphore);
    }

    if (!args.nomodes) {
      printf("*********** Display modes **********\n\n");
      for (;;) {
        if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
        {
            printf("\n*** break\n");
            break;
        }
              
	ULONG len;

        modeid = NextDisplayInfo(modeid);
        if (modeid == INVALID_ID)
            break;

	printf("ModeID 0x%08X ", (unsigned)modeid);
	memset(&ni, 0, sizeof(ni));
	len = GetDisplayInfoData(NULL, (UBYTE *)&ni, sizeof(ni), DTAG_NAME, modeid);
	if (len > 0)
	    printf("%s\n", ni.Name);
	else
	    printf("no NameInfo\n");

	printf("DisplayInfo handle: %p\n", FindDisplayInfo(modeid));
#ifndef NO_CGX_API
	if (CyberGfxBase)
	    printf("IsCyberModeID: %d\n", IsCyberModeID(modeid));
#endif

	memset(&di, 0, sizeof(di));
	len = GetDisplayInfoData(NULL, (UBYTE *)&di, sizeof(di), DTAG_DISP, modeid);
	if (len > 0) {
	    printf    ("DisplayInfo (%u bytes)\n", (unsigned)len);
	    printf    ("  NotAvailable     0x%04X\n",  di.NotAvailable);
	    printf    ("  PropertyFlags    0x%08X\n",  (unsigned)di.PropertyFlags);
	    PrintPoint("  Resolution      "         , &di.Resolution);
	    printf    ("  PixelSpeed       %u\n"    ,  di.PixelSpeed);
	    printf    ("  NumStdSprites    %u\n"    ,  di.NumStdSprites);
	    printf    ("  PaletteRange     %u\n"    ,  di.PaletteRange);
	    PrintPoint("  SpriteResolution"         , &di.SpriteResolution);
	    printf    ("  RedBits          %u\n"    ,  di.RedBits);
	    printf    ("  GreenBits        %u\n"    ,  di.GreenBits);
	    printf    ("  BlueBits         %u\n"    ,  di.BlueBits);
	} else
	    printf("No DisplayInfo\n");

	memset(&dims, 0, sizeof(dims));
	len = GetDisplayInfoData(NULL, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, modeid);
	if (len > 0) {
	    printf        ("DimensionInfo (%u bytes)\n", (unsigned)len);
	    printf        ("  MaxDepth        %u\n",  dims.MaxDepth);
	    printf        ("  MinRasterWidth  %u\n",  dims.MinRasterWidth);
	    printf        ("  MinRasterHeight %u\n",  dims.MinRasterHeight);
	    printf        ("  MaxRasterWidth  %u\n",  dims.MaxRasterWidth);
	    printf        ("  MaxRasterHeight %u\n",  dims.MaxRasterHeight);
	    PrintRectangle("  Nominal        "     , &dims.Nominal);
	    PrintRectangle("  MaxOScan       "     , &dims.MaxOScan);
	    PrintRectangle("  VideoOScan     "     , &dims.VideoOScan);
	    PrintRectangle("  TxtOScan       "     , &dims.TxtOScan);
	    PrintRectangle("  StdOScan       "     , &dims.StdOScan);
	} else
	    printf("No DimensionInfo\n");

	memset(&mon, 0, sizeof(mon));
	len = GetDisplayInfoData(NULL, (UBYTE *)&mon, sizeof(mon), DTAG_MNTR, modeid);
	if (len > 0) {
	    printf        ("MonitorInfo (%u bytes)\n", (unsigned)len);
	    PrintName     ("  Mspc               "         , &mon.Mspc->ms_Node);
	    PrintPoint    ("  ViewPosition       "         , &mon.ViewPosition);
	    PrintPoint    ("  ViewResolution     "         , &mon.ViewResolution);
	    PrintRectangle("  ViewPositionRange  "         , &mon.ViewPositionRange);
	    printf        ("  TotalRows           %u\n"    ,  mon.TotalRows);
	    printf        ("  TotalColorClocks    %u\n"    ,  mon.TotalColorClocks);
	    printf        ("  MinRow              %u\n"    ,  mon.MinRow);
            printf        ("  Compatibility       %d\n"    ,  mon.Compatibility);
	    PrintPoint    ("  MouseTicks         "         , &mon.MouseTicks);
	    PrintPoint    ("  DefaultViewPosition"         , &mon.DefaultViewPosition);
	    printf        ("  PreferredModeID     0x%08X\n",  (unsigned)mon.PreferredModeID);

	    if (args.allspecs)
	        mspc = NULL;
	    else {
	        ObtainSemaphoreShared(GfxBase->MonitorListSemaphore);
	        for (mspc = (struct MonitorSpec *)GfxBase->MonitorList.lh_Head; mspc->ms_Node.xln_Succ; mspc = (struct MonitorSpec *)mspc->ms_Node.xln_Succ) {
	            if (mspc == mon.Mspc)
		        break;
		}
		ReleaseSemaphore(GfxBase->MonitorListSemaphore);
	    }
	    if ((mspc != mon.Mspc) && mon.Mspc)
	        PrintMonitorSpec(mon.Mspc);

	} else
	    printf("No MonitorInfo\n");

	printf("\n");
      }
    }

    printf("*************** End ****************\n");
    if (P96Base)
	CloseLibrary(P96Base);
    if (CyberGfxBase)
	CloseLibrary(CyberGfxBase);
    
    FreeArgs(rda);
    return 0;
}
