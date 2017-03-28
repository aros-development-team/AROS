/*
    Copyright © 2011-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cumulative graphics benchmarks
    Lang: English
*/
/*****************************************************************************

    NAME

        gfxbench

    SYNOPSIS

    LOCATION

    FUNCTION
    
    RESULT

    NOTES

    BUGS

    INTERNALS

******************************************************************************/

#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <devices/timer.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <resources/processor.h>
#include <proto/processor.h>
#include <proto/oop.h>
#include <proto/cybergraphics.h>
#include <hidd/pci.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct Window * win;
LONG            width = 1280;
LONG            height = 720;

/* COMMON */
static void printresults(LONG timems, LONG blits)
{
    LONG bpp; QUAD q;

    bpp = GetCyberMapAttr(win->WScreen->RastPort.BitMap, CYBRMATTR_BPPIX);
    q = ((QUAD)width) * ((QUAD)height) * ((QUAD)bpp) * ((QUAD)blits) * ((QUAD)1000000) / (QUAD)timems;

    printf("%.2f|%d|\n", (blits * 1000000.0 / timems), (int)(q / 1048576));
}

static void cleanup(STRPTR msg, ULONG retcode)
{
    if (msg) 
        fprintf(stderr, "text: %s\n", msg);

    exit(retcode);
}

/* TEXT BENCH */
#define ONLY_BENCH_CODE

STRPTR          consttext = "The AROS Development Team. All rights reserved.";
LONG            mode = JAM1;
BOOL            antialias = FALSE;
LONG            linelen = 100;
LONG            function = 0;
LONG            pixfmt = RECTFMT_ARGB;

#include "text.c"
#include "pixelarray.c"

/* PCI Vendor/Product name translaction. Taken from PCITool */

#include <ctype.h>
// it's supposed to become a shared library one day ...
// 
// current implementation:
// on pciids_Open(), the file is read in memory, then
// an index is built (in computeVendorIndexes()), as an array
// of couples (vendor_id, offset)
// where offset is the offset where the vendor stuff begins
// in the memory file. This array is ascending sorted by vendor_id,
// so a binary search can be done to retrieve the vendor offset
// given its id. This search is done in getVendorIndex().
// All the stringification functions first call this search func,
// then parse the memory:
// 1234  VendorName (so: s[0] = hex digit,  s[4] == ' ', s[6+] == name)
// <tab>1234  DeviceName (same with a tab on linestart)
// todo: subvendor/subdevice parsing
// todo: proper memory reallocation, currently the index is fixed
// to 2500 vendors

static STRPTR mem = NULL;
static ULONG memsize = 0;

struct vendor_cell
{
    UWORD vendorID;
    LONG  offset;
};

static struct vendor_cell *vendor_index = NULL;
static ULONG vi_allocated = 0;
static UWORD vi_number = 0;

static LONG skip_line(const char *buffer, LONG size, LONG pos)
{
    buffer += pos;
    while (pos < size)
    {
        if (*buffer++ == '\n')
        {
            pos++;
            break;
        }
        pos++;
    }
    return pos;
}

static LONG copy_until_eol(STRPTR m, ULONG msize, LONG pos, STRPTR buf,
               ULONG bufsize)
{
    LONG j = 0;

    m += pos;
    while ((pos < msize) && (j < bufsize - 1) && (*m != '\n'))
    {
        buf[j++] = *m++;
    }
    buf[j] = 0;
    return j;
}

static BOOL computeVendorIndexes(const char *buffer, LONG size)
{
    LONG i, j;

    vi_allocated = 2500;
    vendor_index = AllocVec(vi_allocated * sizeof(struct vendor_cell), MEMF_ANY);
    if (NULL == vendor_index)
        return FALSE;

    i = 0;
    j = 0;

    while (i < size)
    {
        // don't use isxdigit, beware of uppercase letter
        if ((isdigit(buffer[i]) || (buffer[i] >= 'a' && buffer[i] <= 'f'))
        && (i + 4 < size) && (buffer[i + 4] == ' '))
        {
            if (sscanf(buffer + i, "%hx", &(vendor_index[j].vendorID)) != 1)
                return FALSE;
            vendor_index[j].offset = i;

            j++;
            if (j >= vi_allocated)
            {
                FreeVec(vendor_index);
                vendor_index = NULL;
                return FALSE;
            }
        }
        i = skip_line(buffer, size, i);
    }
    vi_number = j - 1;
    return TRUE;
}

static LONG getVendorIndex(UWORD vendorID)
{
    LONG lower = 0;
    LONG upper = vi_number;

    if (!mem || !vendor_index)
        return -1;

    while (upper != lower)
    {
        UWORD vid;

        vid = vendor_index[(upper + lower) / 2].vendorID;
        if (vid == vendorID)
            return vendor_index[(upper + lower) / 2].offset;
        if (vendorID > vid)
            lower = (upper + lower) / 2 + 1;
        else
            upper = (upper + lower) / 2;
    }
    return -1;
}

static LONG getDeviceIndex(LONG vendorIndex, UWORD deviceID)
{
    LONG i = vendorIndex;

    if (i < 0)
        return i;

    i = skip_line(mem, memsize, i); // skip vendor
    while ((i < memsize) && ((mem[i] == '\t') || (mem[i] == '#')))
    {
        UWORD did;

        if (mem[i] != '#')
        {
            if ((i + 6 < memsize) && (mem[i + 5] == ' ')
            && (sscanf(mem + i + 1, "%hx", &did) == 1) && (did == deviceID))
            {
                return i;
            }
        }
        i = skip_line(mem, memsize, i);
    }
    return -1;
}

static void pciids_Open(void)
{
    BPTR fh;
    LONG size;

    fh = Open("DEVS:pci.ids", MODE_OLDFILE);
    if (!fh)
        goto err_open_ids;

    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_CURRENT);
    if (size <= 0)
        goto err_size;

    memsize = (ULONG)size;
    Seek(fh, 0, OFFSET_BEGINNING);

    mem = AllocVec(memsize, MEMF_ANY);
    if (NULL == mem)
        goto err_mem;

    if (Read(fh, mem, memsize) != size)
        goto err_read;

    if (!computeVendorIndexes(mem, memsize))
        goto err_index;

    // success !
    return;

err_index:
err_read:
    FreeVec(mem);
    mem = NULL;
err_mem:
err_size:
    Close(fh);
err_open_ids:
    return;
}

static void pciids_Close(void)
{
    if (vendor_index)
    {
        FreeVec(vendor_index);
        vendor_index = NULL;
    }

    if (mem)
    {
        FreeVec(mem);
        mem = NULL;
    }
}

static STRPTR pciids_GetVendorName(UWORD vendorID, STRPTR buf, ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);

    buf[0] = 0;
    if (i < 0)
        return buf;

    copy_until_eol(mem, memsize, i + 6, buf, bufsize);

    return buf;
}

static STRPTR pciids_GetDeviceName(UWORD vendorID, UWORD deviceID, STRPTR buf, ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);

    buf[0] = 0;
    if (i < 0) // unknown vendor
        return buf;

    i = getDeviceIndex(i, deviceID);
    if (i < 0) // unknown device
        return buf;

    copy_until_eol(mem, memsize, i + 7, buf, bufsize);
    return buf;
}

/* PCI Vendor/Product name translaction. Taken from PCITool */



OOP_AttrBase HiddPCIDeviceAttrBase  = 0;
OOP_Object * pciDriver              = NULL;
OOP_Object * pciBus                 = NULL;
struct Library * OOPBase            = NULL;

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, pciDevice, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    IPTR productid;
    IPTR vendorid;
    IPTR agpcap;
    IPTR pciecap;
    TEXT vendor[100] = {0};
    TEXT product[100] = {0};
    
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &productid);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &vendorid);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_CapabilityAGP, (APTR)&agpcap);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_CapabilityPCIE, (APTR)&pciecap);
    pciids_GetVendorName(vendorid, vendor, 100);
    pciids_GetDeviceName(vendorid, productid, product, 100);

    printf("|Video card|0x%x:0x%x %s %s", 
        (unsigned)vendorid, (unsigned)productid, vendor, product);
    if (agpcap) printf(" AGP");
    if (pciecap) printf(" PCIe");
    printf("|\n");

    AROS_USERFUNC_EXIT
}

static void listvideocards()
{
    OOPBase = OpenLibrary("oop.library", 0L);
    if (!OOPBase) return;

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (!pciBus)
    {
        pciBus = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        if (!pciBus)
        {
            CloseLibrary(OOPBase);
            return;
        }
    }

    pciids_Open();

    if (pciBus)
    {
        struct Hook FindHook = {
        h_Entry:    (IPTR (*)())Enumerator,
        h_Data:     NULL,
        };

        struct TagItem Requirements[] = {
        { tHidd_PCI_Interface,  0x00 },
        { tHidd_PCI_Class,      0x03 },
        { tHidd_PCI_SubClass,   0x00 },
        { TAG_DONE,             0UL }
        };
    
        struct pHidd_PCI_EnumDevices enummsg = {
        mID:        OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
        callback:   &FindHook,
        requirements:   (struct TagItem*)&Requirements,
        }, *msg = &enummsg;

        OOP_DoMethod(pciBus, (OOP_Msg)msg);
    }
    
    OOP_DisposeObject(pciBus);
    CloseLibrary(OOPBase);
    pciids_Close();
}

static void detectsystem()
{
    printf("*System information*\n");

    /* Detect CPU + Memory */
    APTR ProcessorBase = OpenResource(PROCESSORNAME);
    
    if (ProcessorBase)
    {
        ULONG processorcount;
        ULONG i;
        struct TagItem tags [] = 
        {
            { GCIT_NumberOfProcessors, (IPTR)&processorcount },
            { 0, (IPTR)NULL }
        };
        
        GetCPUInfo(tags);
        
        printf("|Processor count|%d|\n", (int)processorcount);

        for (i = 0; i < processorcount; i++)
        {
            UQUAD frequency = 0;
            STRPTR modelstr = NULL;

            struct TagItem tags [] = 
            {
                { GCIT_SelectedProcessor, (IPTR)i },
                { GCIT_ProcessorSpeed, (IPTR)&frequency },
                { GCIT_ModelString, (IPTR)&modelstr },
                { TAG_DONE, TAG_DONE }
            };

            GetCPUInfo(tags);

            frequency /= 1000000;
            
            printf("|Processor #%d|%s - %d Mhz|\n", (int)i, modelstr, (int)frequency);
        }
    }
    
    printf("|Available memory|%dkB|\n", (int)(AvailMem(MEMF_ANY) / 1024));
    
    /* Detect video card device */
    listvideocards();
    
    /* Detect screen properties */
    {
        struct Screen * screen = IntuitionBase->FirstScreen;
        LONG sdepth = 0, swidth = 0, sheight = 0;

        swidth = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_WIDTH);
        sheight = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_HEIGHT);
        sdepth = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_DEPTH);
        
        if (width > swidth) width = swidth;
        if (height > sheight) height = sheight;

        printf("|Screen information| %dx%dx%d|\n", (int)swidth, (int)sheight, (int)sdepth);
    }

    printf("\n\n");
}

static void textbenchmark(LONG optmode, BOOL optantialias, LONG optlen)
{
    STRPTR modestr = "UNKNOWN";
    STRPTR aastr = "UNKNOWN";
    TEXT lenstr[10] = {0};
    
    switch(optmode)
    {
    case(JAM1): modestr = "JAM1"; break;
    case(JAM2): modestr = "JAM2"; break;
    case(COMPLEMENT): modestr = "COMPLEMENT"; break;
    }
    mode = optmode;
    
    if (optantialias)
        aastr = "ANTIALIASED";
    else
        aastr = "NON-ANTIALIASED";
    antialias = optantialias;
    
    sprintf(lenstr, "LEN %d", (int)optlen);
    linelen = optlen;
    
    printf("|%s, %s, %s|", modestr, aastr, lenstr);
    
    action_text();
}

static void textbenchmarkset()
{
    printf("*Text benchmark %dx%d*\n", (int)width, (int)height);
    printf("||Test||Blits/s||MB/s||\n");
    textbenchmark(JAM1,         FALSE,  100);
    textbenchmark(JAM2,         FALSE,  100);
    textbenchmark(COMPLEMENT,   FALSE,  100);
    textbenchmark(JAM1,         TRUE,   100);
    textbenchmark(JAM2,         TRUE,   100);
    textbenchmark(COMPLEMENT,   TRUE,   100);
    textbenchmark(JAM1,         FALSE,  5);
    textbenchmark(JAM2,         FALSE,  5);
    textbenchmark(COMPLEMENT,   FALSE,  5);
    textbenchmark(JAM1,         TRUE,   5);
    textbenchmark(JAM2,         TRUE,   5);
    textbenchmark(COMPLEMENT,   TRUE,   5);
    printf("\n\n");
}

static void pixelarraybenchmark(LONG optpixfmt, LONG optfunction)
{
    STRPTR functionstr = "UNKNOWN";
    STRPTR pixfmtstr = "UNKNOWN";
    LONG i;
    
    switch(optfunction)
    {
    case(FUNCTION_WRITE): functionstr = "WritePixelArray"; break;
    case(FUNCTION_READ): functionstr = "ReadPixelArray"; break;
    case(FUNCTION_WRITE_ALPHA): functionstr = "WritePixelArrayAlpha"; break;
    }
    
    for(i = 0; pixfmt_table[i].name; i++)
    {
        if (pixfmt_table[i].id == optpixfmt)
        {
            pixfmtstr = pixfmt_table[i].name;
            break;
        }
    }

    pixfmt = optpixfmt;
    function = optfunction;
    
    printf("| %s %s|", functionstr, pixfmtstr);
    
    action_pixelarray();
}

static void pixelarraybenchmarkset()
{
    printf("*PixelArray benchmark %dx%d*\n", (int)width, (int)height);
    printf("||Test||Blits/s||MB/s||\n");
    pixelarraybenchmark(RECTFMT_RGB,    FUNCTION_WRITE);
    pixelarraybenchmark(RECTFMT_ARGB32, FUNCTION_WRITE);
    pixelarraybenchmark(RECTFMT_RGBA,   FUNCTION_WRITE);
    pixelarraybenchmark(RECTFMT_RGB16PC,FUNCTION_WRITE);
    pixelarraybenchmark(RECTFMT_LUT8,   FUNCTION_WRITE);
    pixelarraybenchmark(RECTFMT_RGB,    FUNCTION_READ);
    pixelarraybenchmark(RECTFMT_ARGB32, FUNCTION_READ);
    pixelarraybenchmark(RECTFMT_RGBA,   FUNCTION_READ);
    pixelarraybenchmark(RECTFMT_RGB16PC,FUNCTION_READ);
    pixelarraybenchmark(RECTFMT_ARGB32, FUNCTION_WRITE_ALPHA);
    printf("\n\n");
}

int main(void)
{
    detectsystem();

    pixelarraybenchmarkset();

    textbenchmarkset();

    cleanup(NULL, 0);
    
    return 0;
}
