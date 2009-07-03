/*
 *----------------------------------------------------------------------------
 *                         Pencam Tool for Poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>

#include "PencamTool.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ARGS_TO       0
#define ARGS_PICNUM   1
#define ARGS_INTERVAL 2
#define ARGS_UPTO     3
#define ARGS_NOBEEP   4
#define ARGS_GAMMA    5
#define ARGS_SHARPEN  6
#define ARGS_TEXT     7
#define ARGS_FONT     8
#define ARGS_FONTSIZE 9
#define ARGS_UNIT     10
#define ARGS_SIZEOF   11

static const char *prgname = "PencamTool";
static const char *template = "TO/A,PICNUM/N,INTERVAL/N,UPTO/N/K,NOBEEP/S,GAMMA/K,SHARPEN/S,TEXT/K,FONT/K,FONTSIZE/N/K,UNIT/N/K";
static const char *version = "$VER: PencamTool 1.7 (12.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

static UWORD gammaredtab[256];
static UWORD gammagreentab[256];
static UWORD gammabluetab[256];

struct RastPort fontrp;
struct RastPort picrp;
struct TextAttr avenirta;
struct TextFont *avenirfont = NULL;
struct BitMap *fontbm = NULL;
ULONG tlength, theight;

struct Library *ps;

AROS_UFP3(void, releasehook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(APTR, pab, A2),
          AROS_UFPA(struct NepClassPencam *, nch, A1));

struct NepClassPencam * SetupPencam(void);
struct NepClassPencam * AllocPencam(struct NepClassPencam *nch);
void FreePencam(struct NepClassPencam *nch);

#define GAMMA 0.450
#define ZERO 17.0

void CreateGammaTab(void)
{
    UWORD i;
    UWORD red, green, blue;
    double x,y;
    double gamma;
    if(!ArgsArray[ARGS_GAMMA])
    {
        return;
    }
    gamma = atof((char *) ArgsArray[ARGS_GAMMA]);
    gammaredtab[0] = gammagreentab[0] = gammabluetab[0] = 0;
    for(i=1; i<256; ++i)
    {
        x = i;
        x -= ZERO;
        if(x < 1.0)
        {
            x = 1.0;
        }
        y = pow((x/256.0), gamma)*255.0;
        red = (UWORD) (y*1.08);
        green = (UWORD) y;
        blue = (UWORD) (y*0.95);
        gammaredtab[i] = red < 256 ? red : 255;
        gammagreentab[i] = green;
        gammabluetab[i] = blue;
    }
}

AROS_UFH3(void, releasehook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, pab, A2),
          AROS_UFHA(struct NepClassPencam *, nch, A1))
{
    AROS_USERFUNC_INIT
    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) prgname,
                   "Pencam killed!");*/
    Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
    AROS_USERFUNC_EXIT
}

struct NepClassPencam * SetupPencam(void)
{
    struct NepClassPencam *nch;
    struct PsdDevice *pd = NULL;
    struct PsdAppBinding *pab;
    ULONG unit;

    if(ArgsArray[ARGS_UNIT])
    {
        unit = *((ULONG *) ArgsArray[ARGS_UNIT]);
    } else {
        unit = 0;
    }
    do
    {
        do
        {
            pd = psdFindDevice(pd,
                               DA_VendorID, 0x0553,
                               DA_ProductID, 0x0202,
                               TAG_END);
        } while(pd && (unit--));

        if(!pd)
        {
            PutStr("No Pencam found!\n");
            return(NULL);
        }
        if((nch = psdAllocVec(sizeof(struct NepClassPencam))))
        {
            nch->nch_Device = pd;
            nch->nch_ReleaseHook.h_Entry = (APTR) releasehook;

            pab = psdClaimAppBinding(ABA_Device, pd,
                                     ABA_ReleaseHook, &nch->nch_ReleaseHook,
                                     ABA_UserData, nch,
                                     TAG_END);
            if(pab)
            {
                if(AllocPencam(nch))
                {
                    return(nch);
                } else {
                    PutStr("Couldn't allocate Pencam...\n");
                }
                psdReleaseAppBinding(pab);
            } else {
                PutStr("Couldn't claim binding!\n");
            }
            psdFreeVec(nch);
        }
        PutStr("Hohum...\n");
    } while(FALSE);
    return(NULL);
}

struct RastPort * SetupText(void)
{
    STRPTR text = (STRPTR) ArgsArray[ARGS_TEXT];
    InitRastPort(&fontrp);
    if(ArgsArray[ARGS_FONT])
    {
        avenirta.ta_Name = (STRPTR) ArgsArray[ARGS_FONT];
        if(ArgsArray[ARGS_FONTSIZE])
        {
            avenirta.ta_YSize = *((ULONG *) ArgsArray[ARGS_FONTSIZE]);
        } else {
            avenirta.ta_YSize = 8;
        }
        avenirta.ta_Style = 0L;
        avenirta.ta_Flags = 0L;
        if(!(avenirfont = OpenDiskFont(&avenirta)))
        {
            Printf("Couldn't open font!\n");
        } else {
            SetFont(&fontrp, avenirfont);
        }
    }
    tlength = TextLength(&fontrp, text, (ULONG) strlen(text));
    theight = fontrp.Font->tf_YSize;
    if(!(fontbm = AllocBitMap(tlength, theight, 1L, BMF_CLEAR, NULL)))
    {
        Printf("Couldn't allocate font bitmap memory (%ldx%ld)", tlength, theight);
        return(NULL);
    }
    fontrp.BitMap = fontbm;
    //printf("String: %s\nLength: %d\n",(char *)argsarray[ARGS_TEXT], tlength);
    Move(&fontrp, 0, (LONG) fontrp.Font->tf_Baseline);
    Text(&fontrp, text, (ULONG) strlen(text));
    return(&fontrp);
}

void FreeText(void)
{
    if(fontbm)
    {
        FreeBitMap(fontbm);
        fontbm = NULL;
    }
    if(avenirfont)
    {
        CloseFont(avenirfont);
        avenirfont = NULL;
    }
}

void PasteText(struct PCImageHeader *pcih, UBYTE *output)
{
    LONG x, y;
    LONG tarx, tary;
    LONG vw = pcih->pcih_ImgWidth;
    LONG vw3 = vw+vw+vw;
    UBYTE *op;
    WORD pix;
    if(tlength < vw-2)
    {
        tarx = (vw-tlength) >> 1;
    } else {
        tarx = 1;
    }
    tary = (pcih->pcih_ImgHeight-theight-theight);
    for(y = 0; y < theight; y++)
    {
        for(x = 0; (x < tlength) && (x+tarx+2 < vw); x++)
        {
            pix = ReadPixel(&fontrp, x, y);
            if(pix)
            {
                op = &output[((tary+y)*vw+tarx+x)*3];
                op[-vw3] >>= 1;
                op[-vw3+1] >>= 1;
                op[-vw3+2] >>= 1;
                op[-3] >>= 1;
                op[-2] >>= 1;
                op[-1] >>= 1;
                op[3] >>= 1;
                op[4] >>= 1;
                op[5] >>= 1;
                op[vw3] >>= 1;
                op[vw3+1] >>= 1;
                op[vw3+2] >>= 1;
            }
        }
    }
    for(y = 0; y < theight; y++)
    {
        for(x = 0; (x < tlength) && (x+tarx+2 < vw); x++)
        {
            pix = ReadPixel(&fontrp, x, y);
            if(pix)
            {
                op = &output[((tary+y)*vw+tarx+x)*3];
                *op++ = 255;
                *op++ = 255;
                *op = 255;
            }
        }
    }
}

struct NepClassPencam * AllocPencam(struct NepClassPencam *nch)
{
    struct List *cfglist;
    struct List *iflist;
    struct List *altiflist;
    IPTR ifnum;
    IPTR altnum;
    struct List *eplist;

    nch->nch_Task = FindTask(NULL);

    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_ConfigList, &cfglist,
                TAG_END);

    if(!cfglist->lh_Head->ln_Succ)
    {
        PutStr("No configs?\n");
        return(NULL);
    }

    nch->nch_Config = (struct PsdConfig *) cfglist->lh_Head;

    psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                CA_InterfaceList, &iflist,
                TAG_END);

    if(!iflist->lh_Head->ln_Succ)
    {
        PutStr("No interfaces?\n");
        return(NULL);
    }

    nch->nch_Interface = (struct PsdInterface *) iflist->lh_Head;
    psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                IFA_InterfaceNum, &ifnum,
                IFA_AlternateNum, &altnum,
                IFA_AlternateIfList, &altiflist,
                IFA_EndpointList, &eplist,
                TAG_END);

    if((nch->nch_TaskMsgPort = CreateMsgPort()))
    {
        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
        {
            if((ifnum == 0) && (altnum == 0))
            {
                psdSetAltInterface(nch->nch_EP0Pipe, altiflist->lh_Head);
            }
            psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                        CA_InterfaceList, &iflist,
                        TAG_END);
            nch->nch_Interface = (struct PsdInterface *) iflist->lh_Head;
            psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                        IFA_InterfaceNum, &ifnum,
                        IFA_AlternateNum, &altnum,
                        IFA_EndpointList, &eplist,
                        TAG_END);
            if(eplist->lh_Head->ln_Succ)
            {
                nch->nch_BulkEP = (struct PsdEndpoint *) eplist->lh_Head;
                psdGetAttrs(PGA_ENDPOINT, nch->nch_BulkEP,
                            EA_MaxPktSize, &nch->nch_BulkPktSize,
                            TAG_END);
                if((nch->nch_BulkPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_BulkEP)))
                {
                    psdSetAttrs(PGA_PIPE, nch->nch_BulkPipe,
                                PPA_AllowRuntPackets, TRUE,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, 5000,
                                TAG_END);
                    return(nch);
                } else {
                    PutStr("Couldn't allocate bulk pipe.\n");
                }
            } else {
                PutStr("No bulk endpoint?\n");
            }
            psdFreePipe(nch->nch_EP0Pipe);
        } else {
             PutStr("Couldn't allocate default pipe\n");
        }
        DeleteMsgPort(nch->nch_TaskMsgPort);
    }
    return(NULL);
}


void FreePencam(struct NepClassPencam *nch)
{
    APTR pab;

    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_Binding, &pab,
                TAG_END);
    psdReleaseAppBinding(pab);
    psdFreePipe(nch->nch_BulkPipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    psdFreeVec(nch);
}

/**************************************************************************/

void bayer_unshuffle(struct PCImageHeader *pcih, UBYTE *raw, UBYTE *output)
{
    ULONG x, y;
    ULONG w = pcih->pcih_ImgWidth>>1;
    ULONG vw = pcih->pcih_ImgWidth;
    ULONG vh = pcih->pcih_ImgHeight;
    UBYTE *raweven;
    UBYTE *rawodd;
    UBYTE *oline;

    for(y = 0; y < vh; y++)
    {
        rawodd = &raw[y*vw];
        raweven = &rawodd[w];
        oline = output;
        if(y & 1)
        {
            ++oline;
        }
        ++oline;
        x = w;
        do
        {
            *oline = *raweven++;
            oline += 2;
            *oline = *rawodd++;
            oline += 4;
        } while(--x);
        output += vw;
        output += vw;
        output += vw;
    }
}

void bayer_demosaic(struct PCImageHeader *pcih, UBYTE *output)
{
    LONG x, y;
    LONG vw = pcih->pcih_ImgWidth;
    LONG vw3 = vw+vw+vw;
    LONG vh = pcih->pcih_ImgHeight;
    UBYTE *op;
    for(y = 1; y < vh-1; y++)
    {
        op = &output[(y*vw + 1)*3];
        for(x = 1; x < vw-1; x++) /* work out pixel type */
        {
            switch(((y + y) & 2) + (x & 1))
            {
                case 0:        /* green. red lr, blue tb */
                    *op = (((UWORD) op[-3]) + ((UWORD) op[3])) >> 1; /* Set red */
                    op[2] = (((UWORD) op[2-vw3]) + ((UWORD) op[2+vw3]) + 1) >> 1; /* Set blue */
                    break;
                case 1:        /* red. green lrtb, blue diagonals */
                    op[1] = (((UWORD) op[-2]) + ((UWORD) op[4]) +
                             ((UWORD) op[1-vw3]) + ((UWORD) op[1+vw3]) + 2) >> 2; /* Set green */
                    op[2] = (((UWORD) op[-1-vw3]) + ((UWORD) op[5-vw3]) +
                             ((UWORD) op[-1+vw3]) + ((UWORD) op[5+vw3]) + 2) >> 2; /* Set blue */
                    break;
                case 2:        /* blue. green lrtb, red diagonals */
                    op[1] = (((UWORD) op[-2]) + ((UWORD) op[4]) +
                             ((UWORD) op[1-vw3]) + ((UWORD) op[1+vw3]) + 2) >> 2; /* Set green */
                    *op = (((UWORD) op[-3-vw3]) + ((UWORD) op[3-vw3]) +
                           ((UWORD) op[-3+vw3]) + ((UWORD) op[3+vw3]) + 2) >> 2; /* Set red */
                    break;
                case 3:        /* green. blue lr, red tb */
                    op[2] = (((UWORD) op[-1]) + ((UWORD) op[5]) + 1) >> 1; /* Set blue */
                    *op = (((UWORD) op[-vw3]) + ((UWORD) op[vw3]) + 1) >> 1; /* Set red */
                    break;
            }  /* switch */
            op += 3;
        }   /* for x */
    }  /* for y */
}

void gammacorrection(struct PCImageHeader *pcih, UBYTE *output)
{
    ULONG cnt = pcih->pcih_ImgWidth*pcih->pcih_ImgHeight;
    while(cnt--)
    {
        *output = gammaredtab[*output];
        output++;
        *output = gammagreentab[*output];
        output++;
        *output = gammabluetab[*output];
        output++;
    }
}

void sharpen5x5(struct PCImageHeader *pcih, UBYTE *input, UBYTE *output)
{
    LONG x, y;
    LONG vw = pcih->pcih_ImgWidth;
    LONG vw3 = vw+vw+vw;
    LONG vw6 = vw*6;
    LONG vh = pcih->pcih_ImgHeight;
    LONG linem2[3];
    LONG linem1[3];
    LONG linep1[3];
    LONG linep2[3];
    LONG val[3];

    UBYTE *op;
    UBYTE *oop;
    for(y = 2; y < vh-2; y++)
    {
        op = &input[((y-2)*vw)*3];
        linem2[0] = *op++;  // -2
        linem2[1] = *op++;
        linem2[2] = *op++;
        linem2[0] += *op++; // -1
        linem2[1] += *op++;
        linem2[2] += *op++;
        linem2[0] += *op++; // 0
        linem2[1] += *op++;
        linem2[2] += *op++;
        linem2[0] += *op++; // 1
        linem2[1] += *op++;
        linem2[2] += *op++;
        linem2[0] += *op++; // 2
        linem2[1] += *op++;
        linem2[2] += *op;

        op = &input[((y-1)*vw+1)*3];
        linem1[0] = *op++;  // -1
        linem1[1] = *op++;
        linem1[2] = *op++;
        linem1[0] += *op++; // 0
        linem1[1] += *op++;
        linem1[2] += *op++;
        linem1[0] += *op++; // 1
        linem1[1] += *op++;
        linem1[2] += *op++;
        linem1[0] += linem1[0]+linem1[0];
        linem1[1] += linem1[1]+linem1[1];
        linem1[2] += linem1[2]+linem1[2];
        linem1[0] += *op++; // 2
        linem1[1] += *op++;
        linem1[2] += *op++;
        linem1[0] += op[-15]; // -2
        linem1[1] += op[-14];
        linem1[2] += op[-13];

        op = &input[((y+1)*vw+1)*3];
        linep1[0] = *op++;  // -1
        linep1[1] = *op++;
        linep1[2] = *op++;
        linep1[0] += *op++; // 0
        linep1[1] += *op++;
        linep1[2] += *op++;
        linep1[0] += *op++; // 1
        linep1[1] += *op++;
        linep1[2] += *op++;
        linep1[0] += linep1[0]+linep1[0];
        linep1[1] += linep1[1]+linep1[1];
        linep1[2] += linep1[2]+linep1[2];
        linep1[0] += *op++; // 2
        linep1[1] += *op++;
        linep1[2] += *op++;
        linep1[0] += op[-15]; // -2
        linep1[1] += op[-14];
        linep1[2] += op[-13];

        op = &input[((y+2)*vw)*3];
        linep2[0] = *op++;  // -2
        linep2[1] = *op++;
        linep2[2] = *op++;
        linep2[0] += *op++; // -1
        linep2[1] += *op++;
        linep2[2] += *op++;
        linep2[0] += *op++; // 0
        linep2[1] += *op++;
        linep2[2] += *op++;
        linep2[0] += *op++; // 1
        linep2[1] += *op++;
        linep2[2] += *op++;
        linep2[0] += *op++; // 2
        linep2[1] += *op++;
        linep2[2] += *op;

        op = &input[(y*vw + 2)*3];
        oop = &output[(y*vw + 2)*3];
        for(x = 2; x < vw-2; x++) /* work out pixel type */
        {
            /* Central line */
            val[0] = op[-3] + op[3];
            val[1] = op[-2] + op[4];
            val[2] = op[-1] + op[5];
            val[0] += val[0] + val[0] + op[-6] + op[6] + linem2[0] + linem1[0] + linep1[0] + linep2[0];
            val[1] += val[1] + val[1] + op[-5] + op[7] + linem2[1] + linem1[1] + linep1[1] + linep2[1];
            val[2] += val[2] + val[2] + op[-4] + op[8] + linem2[2] + linem1[2] + linep1[2] + linep2[2];
            val[0] -= op[0] * 56;
            val[1] -= op[1] * 56;
            val[2] -= op[2] * 56;
#define MAXVAL 4080

            *oop++ = (val[0] > 0) ? 0 : ((val[0] < -MAXVAL) ? 255 : (-val[0]+8)>>4);
            *oop++ = (val[1] > 0) ? 0 : ((val[1] < -MAXVAL) ? 255 : (-val[1]+8)>>4);
            *oop++ = (val[2] > 0) ? 0 : ((val[2] < -MAXVAL) ? 255 : (-val[2]+8)>>4);

            /* Update line y-2 */
            linem2[0] -= op[-vw6-6];
            linem2[0] += op[-vw6+9];
            linem2[1] -= op[-vw6-5];
            linem2[1] += op[-vw6+10];
            linem2[2] -= op[-vw6-4];
            linem2[2] += op[-vw6+11];

            /* Update line y-1 */
            linem1[0] -= op[-vw3-6];
            linem1[0] -= op[-vw3-3]<<1;
            linem1[0] += op[-vw3+6]<<1;
            linem1[0] += op[-vw3+9];
            linem1[1] -= op[-vw3-5];
            linem1[1] -= op[-vw3-2]<<1;
            linem1[1] += op[-vw3+7]<<1;
            linem1[1] += op[-vw3+10];
            linem1[2] -= op[-vw3-4];
            linem1[2] -= op[-vw3-1]<<1;
            linem1[2] += op[-vw3+8]<<1;
            linem1[2] += op[-vw3+11];

            /* Update line y+1 */
            linep1[0] -= op[vw3-6];
            linep1[0] -= op[vw3-3]<<1;
            linep1[0] += op[vw3+6]<<1;
            linep1[0] += op[vw3+9];
            linep1[1] -= op[vw3-5];
            linep1[1] -= op[vw3-2]<<1;
            linep1[1] += op[vw3+7]<<1;
            linep1[1] += op[vw3+10];
            linep1[2] -= op[vw3-4];
            linep1[2] -= op[vw3-1]<<1;
            linep1[2] += op[vw3+8]<<1;
            linep1[2] += op[vw3+11];

            /* Update line y-2 */
            linep2[0] -= op[vw6-6];
            linep2[0] += op[vw6+9];
            linep2[1] -= op[vw6-5];
            linep2[1] += op[vw6+10];
            linep2[2] -= op[vw6-4];
            linep2[2] += op[vw6+11];

            op += 3;
        }
    }
}

APTR TransferImage(struct NepClassPencam *nch, struct PCImageHeader *pcih)
{
    LONG ioerr;
    UBYTE *rawbuf;
    UBYTE *imgbuf;
    UBYTE *newimgbuf;

    rawbuf = psdAllocVec(pcih->pcih_ImgSize);
    if(!rawbuf)
    {
        Printf("Couldn't allocate %ld bytes of memory.\n", pcih->pcih_ImgSize);
        return(NULL);
    }
    /* Workaround for a firmware bug */
    ioerr = psdDoPipe(nch->nch_BulkPipe, rawbuf, 64);
    if(!ioerr)
    {
        if(((ULONG *) rawbuf)[0] == AROS_LONG2BE(0xed15ed15))
        {
            /* Junk packet at the beginning! */
            ioerr = psdDoPipe(nch->nch_BulkPipe, rawbuf, pcih->pcih_ImgSize);
        } else {
            ioerr = psdDoPipe(nch->nch_BulkPipe, &rawbuf[64], pcih->pcih_ImgSize-64);
        }
    }
    if(!ioerr)
    {
        imgbuf = psdAllocVec((ULONG) pcih->pcih_ImgWidth * (ULONG) pcih->pcih_ImgHeight * 3);
        if(imgbuf)
        {
            bayer_unshuffle(pcih, rawbuf, imgbuf);
            psdFreeVec(rawbuf);
            bayer_demosaic(pcih, imgbuf);
            if(ArgsArray[ARGS_SHARPEN])
            {
                newimgbuf = psdAllocVec((ULONG) pcih->pcih_ImgWidth * (ULONG) pcih->pcih_ImgHeight * 3);
                if(newimgbuf)
                {
                    sharpen5x5(pcih, imgbuf, newimgbuf);
                    psdFreeVec(imgbuf);
                    imgbuf = newimgbuf;
                }
            }
            if(ArgsArray[ARGS_GAMMA])
            {
                gammacorrection(pcih, imgbuf);
            }
            if(ArgsArray[ARGS_TEXT])
            {
                PasteText(pcih, imgbuf);
            }
            return(imgbuf);
        } else {
            Printf("Couldn't allocate %ld bytes of memory.\n", pcih->pcih_ImgSize);
        }
    } else {
        Printf("Bulk transfer failed: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    psdFreeVec(rawbuf);
    return(NULL);
}

APTR GetPicture(struct NepClassPencam *nch, ULONG picnum, struct PCImageHeader *pcih)
{
    struct PsdPipe *pp;
    LONG ioerr;

    pp = nch->nch_EP0Pipe;
    psdPipeSetup(pp, URTF_IN|URTF_VENDOR|URTF_DEVICE,
                 CMDID_GET_IMAGE_HEADER, picnum, 0);
    ioerr = psdDoPipe(pp, pcih, sizeof(struct PCImageHeader));
    if(ioerr)
    {
        Printf("GET_IMAGE_HEADER failed: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(NULL);
    }
    
    psdPipeSetup(pp, URTF_OUT|URTF_VENDOR|URTF_DEVICE,
                 CMDID_UPLOAD_IMAGE, picnum, 0);
    ioerr = psdDoPipe(pp, pcih, sizeof(struct PCImageHeader));
    if(!ioerr)
    {
        /* endianess conversion */
        pcih->pcih_ImgSize = AROS_BE2LONG(pcih->pcih_ImgSize);
        pcih->pcih_ImgWidth = AROS_BE2WORD(pcih->pcih_ImgWidth);
        pcih->pcih_ImgHeight = AROS_BE2WORD(pcih->pcih_ImgHeight);
        pcih->pcih_FineExp = AROS_BE2WORD(pcih->pcih_FineExp);
        pcih->pcih_CoarseExp = AROS_BE2WORD(pcih->pcih_CoarseExp);

        return(TransferImage(nch, pcih));
    } else {
        Printf("UPLOAD_IMAGE failed: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    return(NULL);
}

APTR GetVideoSnap(struct NepClassPencam *nch, struct PCImageHeader *pcih)
{
    struct PsdPipe *pp;
    LONG ioerr;

    pp = nch->nch_EP0Pipe;
    psdPipeSetup(pp, URTF_IN|URTF_VENDOR|URTF_DEVICE,
                 CMDID_GRAB_UPLOAD, ArgsArray[ARGS_NOBEEP] ? 0x6000L : 0x2000L, 0);
    ioerr = psdDoPipe(pp, pcih, 8);
    if(ioerr)
    {
        Printf("GRAB_UPLOAD failed: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(NULL);
    }

    /* endianess conversion */
    pcih->pcih_ImgSize = AROS_BE2LONG(pcih->pcih_ImgSize);
    pcih->pcih_ImgWidth = AROS_BE2WORD(pcih->pcih_ImgWidth);
    pcih->pcih_ImgHeight = AROS_BE2WORD(pcih->pcih_ImgHeight);
    pcih->pcih_FineExp = AROS_BE2WORD(pcih->pcih_FineExp);
    pcih->pcih_CoarseExp = AROS_BE2WORD(pcih->pcih_CoarseExp);

    return(TransferImage(nch, pcih));
}

int main(int argc, char *argv[])
{
    struct NepClassPencam *nch;
    struct PCImageHeader pcih;
    BPTR outfile;
    UBYTE *imgbuf;
    ULONG sigs;
    char buf[256];
    ULONG imgcount;
    ULONG picnum;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        PutStr("Wrong arguments!\n");
        return(RETURN_FAIL);
    }
    ps = OpenLibrary("poseidon.library", 1);
    if(!ps)
    {
        FreeArgs(ArgsHook);
        return(RETURN_FAIL);
    }
    if(ArgsArray[ARGS_TEXT])
    {
        if(!(SetupText()))
        {
            FreeArgs(ArgsHook);
            CloseLibrary(ps);
            return(RETURN_ERROR);
        }
    }
    if(!(nch = SetupPencam()))
    {
        FreeText();
        FreeArgs(ArgsHook);
        CloseLibrary(ps);
        return(RETURN_ERROR);
    }
    CreateGammaTab();
    imgcount = 0;
    if(ArgsArray[ARGS_PICNUM])
    {
        picnum = *((ULONG *) ArgsArray[ARGS_PICNUM]);
    } else {
        picnum = 0;
    }
    do
    {
         imgbuf = NULL;
         if(ArgsArray[ARGS_PICNUM])
         {
             imgbuf = GetPicture(nch, picnum, &pcih);
         } else {
             imgbuf = GetVideoSnap(nch, &pcih);
         }
         if(imgbuf)
         {
             psdSafeRawDoFmt(buf, 256, (STRPTR) ArgsArray[ARGS_TO], imgcount);
             outfile = Open(buf, MODE_NEWFILE);
             if(outfile)
             {
                 UWORD y;
                 ULONG h = pcih.pcih_ImgHeight;
                 ULONG vh = h-4;
                 ULONG w = pcih.pcih_ImgWidth*3;
                 ULONG vw = w-4*3;
                 for(y = 0; y < vh; y++)
                 {
                     memcpy(imgbuf+y*vw, imgbuf+(y+2)*w+6, (size_t) vw);
                 }
                 FPrintf(outfile, "P6\n%ld %ld\n255\n", vw/3, vh);
                 Flush(outfile);
                 Write(outfile, imgbuf, vw*vh);
                 Close(outfile);
                 Printf("Wrote image into '%s'.\n", buf);
                 imgcount++;
             } else {
                 Printf("Could not open file '%s' for writing!\n", buf);
             }
             psdFreeVec(imgbuf);
         } else {
             break;
         }
         sigs = SetSignal(0, 0);
         if(sigs & SIGBREAKF_CTRL_C)
         {
             break;
         }
         if(ArgsArray[ARGS_INTERVAL])
         {
             if((!ArgsArray[ARGS_PICNUM]) && ArgsArray[ARGS_UPTO])
             {
                 if(imgcount > *((ULONG *) ArgsArray[ARGS_UPTO]))
                 {
                     break;
                 }
             }
             Delay(*((ULONG *) ArgsArray[ARGS_INTERVAL]));
         } else {
             if(ArgsArray[ARGS_UPTO])
             {
                 if(picnum < *((ULONG *) ArgsArray[ARGS_UPTO]))
                 {
                     picnum++;
                 } else {
                     break;
                 }
             } else {
                 break;
             }
         }
         sigs = SetSignal(0, 0);
         if(sigs & SIGBREAKF_CTRL_C)
         {
             break;
         }
    } while(TRUE);
    FreeText();
    FreePencam(nch);
    FreeArgs(ArgsHook);
    CloseLibrary(ps);
    return(RETURN_OK);
}
