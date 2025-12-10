/*
** DenebClockport V0.0 (23-Jan-08) by Chris Hodges <hodges@in.tum.de>
**
** Project started: 23-Jan-08
** Releases       :
*/

#if !defined(__AROS__)
#define __NOLIBBASE__
#else
#define __saveds
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <dos/dos.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/dos.h>

#include "deneb.h"

#define MACH_CLOCKPORT_Z2   (0x0200>>1) // WORD offset
#define MACH_CLKPORTCTRL_Z2 (0x02fc>>1) // WORD offset

#define MCPCB_FAST_Z2        15    /* Enable Fast Zorro II */
#define MCPCB_RESET_Z2        2    /* Clockport reset */
#define MCPCB_INTEN_Z2        1    /* Enable interrupts */
#define MCPCB_INT2_Z2         0    /* Use INT2 instead of INT6 */

#define MCPCF_FAST_Z2        (1UL<<MCPCB_FAST_Z2)
#define MCPCF_RESET_Z2       (1UL<<MCPCB_RESET_Z2)
#define MCPCF_INTEN_Z2       (1UL<<MCPCB_INTEN_Z2)
#define MCPCF_INT2_Z2        (1UL<<MCPCB_INT2_Z2)

#define	VERSION		1
#define	REVISION	1
#define	DATE	"30.03.08"
#define	VERS	"DenebClockport 1.1"
#define	VSTRING	"DenebClockport 1.1 (30.03.08)"
#define	VERSTAG	"\0$VER: DenebClockport 1.1 (30.03.08) © 2008 Written by Chris Hodges"

struct CPConfig;

ULONG AddClockport(struct CPConfig *cpc);
__saveds void romstart(void);

struct CPNode
{
    UWORD  cpn_VendorID;
    UWORD  cpn_ProductID;
    UWORD  cpn_Offset;
    STRPTR cpn_Name;
};

struct CPNode cpboards[];

#define ARGS_BOARD          0
#define ARGS_CS             1
#define ARGS_AS             2
#define ARGS_RP             3
#define ARGS_WP             4
#define ARGS_SIZEOF         5

struct CPConfig
{
    UWORD cpc_VendorID;
    UWORD cpc_ProductID;
    UWORD cpc_Offset;
    UWORD cpc_CSDelay;
    UWORD cpc_AddressSetup;
    UWORD cpc_ReadPulse;
    UWORD cpc_WritePulse;
};

ULONG main(void)
{
    struct Library *DOSBase;
    struct CPConfig cpc;
    struct CPNode *cpn = cpboards;
    struct RDArgs *ArgsHook;
    LONG ArgsArray[ARGS_SIZEOF] = { 0, 0, 0, 0, 0 };
    char *template = "BOARD,CSDELAY=CS/K/N,ADDRESSSETUP=AS/K/N,READPULSE=RP/K/N,WRITEPULSE=WP/K/N";
    ULONG retval;

    DOSBase = OpenLibrary("dos.library", 36);
    if(!DOSBase)
    {
        return(RETURN_FAIL);
    }
    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        PutStr("Wrong arguments!\n");
        CloseLibrary(DOSBase);
        return(RETURN_FAIL);
    }
    cpc.cpc_Offset = cpn->cpn_Offset;
    cpc.cpc_CSDelay = 0x03;
    cpc.cpc_AddressSetup = 0x03;
    cpc.cpc_ReadPulse = 0x14;
    cpc.cpc_WritePulse = 0x14;

    if(ArgsArray[ARGS_BOARD])
    {
        while(cpn->cpn_Name)
        {
            STRPTR bname = (STRPTR) ArgsArray[ARGS_BOARD];
            STRPTR cptr = cpn->cpn_Name;
            while(*bname && *cptr)
            {
                if((*bname & 0xcf) != (*cptr & 0xcf))
                {
                    break;
                }
                bname++;
                cptr++;
            }
            if(!(*cptr || *bname))
            {
                break;
            }
            cpn++;
        }
        if(!cpn->cpn_Name)
        {
            Printf("Wrong board name '%s', using default!\n", ArgsArray[ARGS_BOARD]);
            cpn = cpboards;
        }
    }
    cpc.cpc_VendorID = cpn->cpn_VendorID;
    cpc.cpc_ProductID = cpn->cpn_ProductID;

    if(ArgsArray[ARGS_CS])
    {
        cpc.cpc_CSDelay = *((ULONG *) ArgsArray[ARGS_CS]);
    }
    if(ArgsArray[ARGS_AS])
    {
        cpc.cpc_AddressSetup = *((ULONG *) ArgsArray[ARGS_AS]);
    }
    if(ArgsArray[ARGS_RP])
    {
        cpc.cpc_ReadPulse = *((ULONG *) ArgsArray[ARGS_RP]);
    }
    if(ArgsArray[ARGS_WP])
    {
        cpc.cpc_WritePulse = *((ULONG *) ArgsArray[ARGS_WP]);
    }
    retval = AddClockport(&cpc);
    FreeArgs(ArgsHook);
    CloseLibrary(DOSBase);
    return(retval);
}

static const char prgname[]     = VERS;
static const char prgidstring[] = VSTRING;
static const char prgverstag[]  = VERSTAG;

struct CPNode cpboards[] =
{
    { 0x1212, 0x0017, 0xa001, "XSurf" }, /* X-Surf CP 0 */
    { 0x1212, 0x0017, 0xc000, "XSurf2" }, /* X-Surf CP 1 */
    { 0x0861, 0x00c8, 0x4000, "Highway" }, /* Highway */
    { 0x0861, 0x0081, 0x0001, "Unity" }, /* Unity Prototype CP 0 */
    { 0x0861, 0x0081, 0x0041, "Unity2" }, /* Unity Prototype CP 1 */
    { 0x1212, 0x0005, 0x8000, "ISDNSurfer" }, /* ISDN-Surfer */
    { 0x1212, 0x0007, 0x9000, "VarIO" }, /* VarIO */
    { 0x1212, 0x0000, 0x0e00, "Buddha" }, /* Buddha flash */
    { 0x1212, 0x000a, 0x8000, "Kickflash" }, /* Kickflash */
    { 0x0000, 0x0000, 0x0000, NULL }
};


static
const struct Resident ROMTag =
{
    RTC_MATCHWORD,
    (struct Resident *) &ROMTag,
    (struct Resident *) (&ROMTag + 1),
#ifdef __MORPHOS__
    RTF_PPC|RTF_COLDSTART,
#else /* __MORPHOS__ */
    RTF_COLDSTART,
#endif /* __MORPHOS__ */
    VERSION,
    NT_UNKNOWN,
    103,
    (char *) prgname,
    (char *) prgidstring,
    (APTR) &romstart
};

/* ---------------------------------------------------------------------- */

__saveds void romstart(void)
{
    struct CPConfig cpc;
    cpc.cpc_VendorID = cpboards->cpn_VendorID;
    cpc.cpc_ProductID = cpboards->cpn_ProductID;
    cpc.cpc_Offset = cpboards->cpn_Offset;
    cpc.cpc_CSDelay = 0x03;
    cpc.cpc_AddressSetup = 0x03;
    cpc.cpc_ReadPulse = 0x14;
    cpc.cpc_WritePulse = 0x14;
    AddClockport(&cpc);
}

/* /// "AddClockport()" */
__saveds ULONG AddClockport(struct CPConfig *cpc)
{
    struct ConfigDev *confdev = NULL;
    struct ConfigDev *cpconfdev;
    struct Library *ExpansionBase;
    ULONG retval = RETURN_WARN;
    volatile ULONG *hwbase;
    ULONG clkbits;

    if(!(ExpansionBase = OpenLibrary("expansion.library", 36)))
    {
        return(RETURN_FAIL);
    }

    while(confdev = FindConfigDev(confdev, 0xe3b, 0x10)) /* Deneb: E3B, Product No 0x10 (Zorro III) */
    {
        cpconfdev = (struct ConfigDev *) confdev->cd_Unused[0];
        if(!cpconfdev)
        {
            // FIXME check if clockport is available at all!
            cpconfdev = AllocConfigDev();
            if(!cpconfdev)
            {
                break;
            }
            cpconfdev->cd_Flags = CDF_CONFIGME;
            cpconfdev->cd_Rom.er_Type = ERT_ZORROIII|(1<<ERT_MEMBIT);
            cpconfdev->cd_Rom.er_Flags =  ERFF_ZORRO_III;
            cpconfdev->cd_Rom.er_Reserved03 = 0x00;
            cpconfdev->cd_Rom.er_SerialNumber = 0xFA1CB0AD;
            cpconfdev->cd_Rom.er_InitDiagVec = 0;
            cpconfdev->cd_Rom.er_Reserved0c = 0;
            cpconfdev->cd_Rom.er_Reserved0d = 0;
            cpconfdev->cd_Rom.er_Reserved0e = 0;
            cpconfdev->cd_Rom.er_Reserved0f = 0;
            cpconfdev->cd_BoardSize = 64<<10;
            cpconfdev->cd_SlotAddr = confdev->cd_SlotAddr;
            cpconfdev->cd_SlotSize = confdev->cd_SlotSize;
            cpconfdev->cd_Driver = cpconfdev; // mark as used
            cpconfdev->cd_NextCD = NULL;
            AddConfigDev(cpconfdev);
            confdev->cd_Unused[0] = (ULONG) cpconfdev;
        }
        cpconfdev->cd_Rom.er_Manufacturer = cpc->cpc_VendorID;
        cpconfdev->cd_Rom.er_Product = cpc->cpc_ProductID;
#if defined(MACH_CLOCKPORT)
        cpconfdev->cd_BoardAddr = ((UBYTE *) confdev->cd_BoardAddr) + (MACH_CLOCKPORT<<2) - (cpc->cpc_Offset & 1);
#else
        cpconfdev->cd_BoardAddr = ((UBYTE *) confdev->cd_BoardAddr) + (MACH_CLOCKPORT_Z2<<1) - cpc->cpc_Offset;
#endif
        hwbase = (ULONG *) confdev->cd_BoardAddr;
        clkbits = MCPCF_INTEN;
        clkbits |= (cpc->cpc_CSDelay<<MCPCS_CSDELAY) & MCPCM_CSDELAY;
        clkbits |= (cpc->cpc_AddressSetup<<MCPCS_ADRSETUP) & MCPCM_ADRSETUP;
        clkbits |= (cpc->cpc_ReadPulse<<MCPCS_READPULSE) & MCPCM_READPULSE;
        clkbits |= (cpc->cpc_WritePulse<<MCPCS_WRITEPULSE) & MCPCM_WRITEPULSE;
        hwbase[MACH_CLKPORTCTRL] = clkbits;

        retval = RETURN_OK;
    }

    while(confdev = FindConfigDev(confdev, 0xe3b, 0x12)) /* Deneb: E3B, Product No 0x12 (Zorro II) */
    {
        cpconfdev = (struct ConfigDev *) confdev->cd_Unused[0];
        if(!cpconfdev)
        {
            // FIXME check if clockport is available at all!
            cpconfdev = AllocConfigDev();
            if(!cpconfdev)
            {
                break;
            }
            cpconfdev->cd_Flags = CDF_CONFIGME;
            cpconfdev->cd_Rom.er_Type = ERT_ZORROII|(1<<ERT_MEMBIT);
            cpconfdev->cd_Rom.er_Flags =  0;
            cpconfdev->cd_Rom.er_Reserved03 = 0x00;
            cpconfdev->cd_Rom.er_SerialNumber = 0xFA1CB0AD;
            cpconfdev->cd_Rom.er_InitDiagVec = 0;
            cpconfdev->cd_Rom.er_Reserved0c = 0;
            cpconfdev->cd_Rom.er_Reserved0d = 0;
            cpconfdev->cd_Rom.er_Reserved0e = 0;
            cpconfdev->cd_Rom.er_Reserved0f = 0;
            cpconfdev->cd_BoardSize = 64<<10;
            cpconfdev->cd_SlotAddr = confdev->cd_SlotAddr;
            cpconfdev->cd_SlotSize = confdev->cd_SlotSize;
            cpconfdev->cd_Driver = cpconfdev; // mark as used
            cpconfdev->cd_NextCD = NULL;
            AddConfigDev(cpconfdev);
            confdev->cd_Unused[0] = (ULONG) cpconfdev;
        }
        cpconfdev->cd_Rom.er_Manufacturer = cpc->cpc_VendorID;
        cpconfdev->cd_Rom.er_Product = cpc->cpc_ProductID;
        cpconfdev->cd_BoardAddr = ((UBYTE *) confdev->cd_BoardAddr) + (MACH_CLOCKPORT_Z2<<1) - cpc->cpc_Offset;
        hwbase = (ULONG *) confdev->cd_BoardAddr;
        hwbase[MACH_CLKPORTCTRL_Z2] |= MCPCF_INTEN_Z2;

        retval = RETURN_OK;
    }

    CloseLibrary(ExpansionBase);
    return(retval);
}
/* \\\ */

