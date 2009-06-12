/*
 *----------------------------------------------------------------------------
 *                         Sonixcam Tool for Poseidon
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

#include "SonixcamTool.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ARGS_TO       0
#define ARGS_INTERVAL 1
#define ARGS_UPTO     2
#define ARGS_GAMMA    3
#define ARGS_SHARPEN  4
#define ARGS_TEXT     5
#define ARGS_FONT     6
#define ARGS_FONTSIZE 7
#define ARGS_UNIT     8
#define ARGS_SIZEOF   9

static const char *prgname = "SonixcamTool";
static const char *template = "TO/A,INTERVAL/N,UPTO/N/K,GAMMA/K,SHARPEN/S,TEXT/K,FONT/K,FONTSIZE/N/K,UNIT/N/K";
static const char *version = "$VER: SonixcamTool 1.3 (12.06.09) by Chris Hodges <chrisly@platon42.de>";
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
          AROS_UFPA(struct NepClassSonixcam *, nch, A1));

AROS_UFP3(void, nInReqHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFPA(struct IOUsbHWBufferReq *, ubr, A1));

AROS_UFP3(void, nInDoneHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFPA(struct IOUsbHWBufferReq *, ubr, A1));

struct NepClassSonixcam * SetupSonixcam(void);
struct NepClassSonixcam * AllocSonixcam(struct NepClassSonixcam *nch);
void FreeSonixcam(struct NepClassSonixcam *nch);

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
          AROS_UFHA(struct NepClassSonixcam *, nch, A1))
{
    AROS_USERFUNC_INIT
    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) prgname,
                   "Sonixcam killed!");*/
    Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, nInReqHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT

    struct NepClassSonixcam *nch = (struct NepClassSonixcam *) hook->h_Data;

    ubr->ubr_Buffer = (UBYTE *) &nch->nch_CurrIsoBuf[nch->nch_IsoBufPos];
    if(ubr->ubr_Length + nch->nch_IsoBufPos > nch->nch_RawBufSize)
    {
        ubr->ubr_Length = 0;
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, nInDoneHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT

    struct NepClassSonixcam *nch = (struct NepClassSonixcam *) hook->h_Data;

    UBYTE *bufptr = &nch->nch_CurrIsoBuf[nch->nch_IsoBufPos];
    //KPRINTF(10, ("%ld\n", ubr->ubr_Length));
    nch->nch_FrameCnt++;
    if(!ubr->ubr_Length)
    {
        if(nch->nch_IsoBufPos > nch->nch_HeaderSize)
        {
            if(nch->nch_IsoBufPos == 320*240+nch->nch_HeaderSize)
            {
                // image done
                nch->nch_LastDoneNum = nch->nch_IsoBufNum;
                nch->nch_BufState[nch->nch_IsoBufNum] = BUF_READY;
                Signal(nch->nch_Task, 1UL<<nch->nch_ImgDoneSig);
                if(++nch->nch_IsoBufNum == 3)
                {
                    nch->nch_IsoBufNum = 0;
                }
                if(nch->nch_BufState[nch->nch_IsoBufNum] == BUF_BUSY)
                {
                    if(++nch->nch_IsoBufNum == 3)
                    {
                        nch->nch_IsoBufNum = 0;
                    }
                }
                KPRINTF(10, ("Next Img: %ld\n", nch->nch_IsoBufNum));
                nch->nch_CurrIsoBuf = nch->nch_RawBuf[nch->nch_IsoBufNum];
            } else {
                KPRINTF(10, ("Restart at %ld/%ld\n", nch->nch_IsoBufPos, nch->nch_FrameCnt));
            }
            nch->nch_IsoBufPos = 0;
        }
        return;
    }
    if(nch->nch_IsoBufPos)
    {
        nch->nch_IsoBufPos += ubr->ubr_Length;
    } else {
        if((*((ULONG *) bufptr) == 0xffff00c4) && (*((UWORD *) &bufptr[4]) == 0xc496))
        {
            //KPRINTF(10, ("Started!\n"));
            nch->nch_IsoBufPos += ubr->ubr_Length;
            nch->nch_FrameCnt = 0;
        }
    }

    AROS_USERFUNC_EXIT
}

BOOL WriteReg(struct NepClassSonixcam *nch, ULONG index, ULONG value)
{
    LONG ioerr;
    UBYTE buf = value;

    //Printf("Writing register 0x%02lx with value 0x%02lx\n", index, value);
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_WRITE_REG, index, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, &buf, 1);
    if(ioerr)
    {
        Printf("Error writing register 0x%02lx with value 0x%02lx, errorcode=%ld\n", index, value, ioerr);
        return FALSE;
    }
    nch->nch_Reg[index & 0xff] = value;
    return TRUE;
}

BOOL WriteRegs(struct NepClassSonixcam *nch, ULONG index, ULONG len, UBYTE *buf)
{
    LONG ioerr;
    ULONG cnt;

    //Printf("Writing registers 0x%02lx with value 0x%02lx\n", index, *buf);
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_WRITE_REG, index, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, len);
    if(ioerr)
    {
        Printf("Error writing registers 0x%02lx - 0x%02lx, errorcode=%ld\n", index, index+len-1, ioerr);
        return FALSE;
    }
    for(cnt = 0; cnt < len; cnt++)
    {
        nch->nch_Reg[(index + cnt) & 0xff] = buf[cnt];
    }
    return TRUE;
}

ULONG ReadReg(struct NepClassSonixcam *nch, ULONG index)
{
    LONG ioerr;
    UBYTE buf;

    //Printf("Reading register 0x%02lx\n", index);
    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_READ_REG, index, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, &buf, 1);
    if(ioerr)
    {
        Printf("Error reading register 0x%02lx, errorcode=%ld\n", index, ioerr);
        return -1;
    }
    return buf;
}

BOOL WaitI2C(struct NepClassSonixcam *nch, BOOL read)
{
    UWORD cnt = 5;
    ULONG reg;
    UBYTE cmpval = read ? 0x04 : 0x0c;

    do
    {
        reg = ReadReg(nch, SXREG_I2C_CTRL);
        if(reg > 255)
        {
            return FALSE;
        }
        if((reg & 0x0c) == cmpval)
        {
            Printf("%s I2C error %02lx\n", read ? "Read" :  "Write", reg);
            return FALSE;
        }
        if(reg & 0x04)
        {
            return TRUE;
        }
        psdDelayMS(3);
    } while(--cnt);
    PutStr("I2C timeout\n");
    return FALSE;
}

BOOL WriteI2C_Raw(struct NepClassSonixcam *nch, UBYTE *cmd)
{
    LONG ioerr;
    BOOL res;
    /*Printf("Writing RAW I2C: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
           cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);*/

    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_WRITE_REG, SXREG_I2C_CTRL, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd, 8);
    if(ioerr)
    {
        Printf("Error writing I2C Reg 0x%2lx with value 0x%02lx, errorcode=%ld\n", cmd[2], cmd[3], ioerr);
        return FALSE;
    }
    res = WaitI2C(nch, FALSE);
    if(!res)
    {
        Printf("Error writing I2C Reg 0x%2lx with value 0x%02lx during wait\n", cmd[2], cmd[3]);
    }
    return(res);
}

BOOL WriteI2C(struct NepClassSonixcam *nch, ULONG index, ULONG value)
{
    UBYTE cmd[8];

    cmd[0] = 0xa0;
    cmd[1] = nch->nch_I2CAddr;
    cmd[2] = index;
    cmd[3] = value;
    cmd[4] = 0;
    cmd[5] = 0;
    cmd[6] = 0;
    cmd[7] = 0x17;
    return(WriteI2C_Raw(nch, cmd));
}

ULONG ReadI2C(struct NepClassSonixcam *nch, ULONG index)
{
    LONG ioerr;
    UBYTE cmd[8];

    cmd[0] = 0x90;
    cmd[1] = nch->nch_I2CAddr;
    cmd[2] = index;
    cmd[3] = 0;
    cmd[4] = 0;
    cmd[5] = 0;
    cmd[6] = 0;
    cmd[7] = 0x10;
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_WRITE_REG, SXREG_I2C_CTRL, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd, 8);
    if(ioerr)
    {
        Printf("Error reading I2C Reg 0x%2lx (phase 1), errorcode=%ld\n", index, ioerr);
        return FALSE;
    }
    if(!WaitI2C(nch, FALSE))
    {
        return FALSE;
    }

    cmd[0] = 0xa2;
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_WRITE_REG, SXREG_I2C_CTRL, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd, 8);
    if(ioerr)
    {
        Printf("Error reading I2C Reg 0x%2lx (phase 2), errorcode=%ld\n", index, ioerr);
        return FALSE;
    }

    if(!WaitI2C(nch, TRUE))
    {
        return FALSE;
    }

    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE,
                 CMDID_READ_REG, SXREG_I2C_CTRL, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd, 8);
    if(ioerr)
    {
        Printf("Error reading I2C Regs 0x%2lx (phase 3), errorcode=%ld\n", index, ioerr);
        return FALSE;
    }
    /*Printf("%02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
           cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);*/
    return cmd[5];
}

UBYTE initOv7630[] =
{
    // 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
    0x00, 0x04, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x03, 0x0a, 0x28, 0x1e, 0x68, 0xC9, 0x20, 0x1d, 0x10, 0x02, 0x03, 0x0f, 0x0c
    //0x00, 0x00, 0x02, 0x03, 0x0a, 0x28, 0x1e, 0x68, 0xc9, 0x50, 0x1d, 0x10, 0x02, 0x03, 0x0f, 0x0c
     //shift one pixel 0x02 is 0x01 at start
};

UBYTE ov7630_sensor_init[] =
{
    0xa0, 0x21, 0x12, 0x80, 0x00, 0x00, 0x00, 0x10,
    0xb0, 0x21, 0x01, 0x77, 0x3a, 0x00, 0x00, 0x10,
    0xd0, 0x21, 0x12, 0x78, 0x00, 0x80, 0x34, 0x10,
    0xa0, 0x21, 0x1b, 0x04, 0x00, 0x80, 0x34, 0x10,
    0xa0, 0x21, 0x20, 0x44, 0x00, 0x80, 0x34, 0x10,
    0xa0, 0x21, 0x23, 0xee, 0x00, 0x80, 0x34, 0x10,
    0xd0, 0x21, 0x26, 0xa0, 0x9a, 0xa0, 0x30, 0x10,
    0xb0, 0x21, 0x2a, 0x80, 0x00, 0xa0, 0x30, 0x10,
    0xb0, 0x21, 0x2f, 0x3d, 0x24, 0xa0, 0x30, 0x10,
    0xa0, 0x21, 0x32, 0x86, 0x24, 0xa0, 0x30, 0x10,
    0xb0, 0x21, 0x60, 0xa9, 0x42, 0xa0, 0x30, 0x10,
    0xa0, 0x21, 0x65, 0x00, 0x42, 0xa0, 0x30, 0x10,
    0xa0, 0x21, 0x69, 0x38, 0x42, 0xa0, 0x30, 0x10,
    0xc0, 0x21, 0x6f, 0x88, 0x0b, 0x00, 0x30, 0x10,
    0xc0, 0x21, 0x74, 0x21, 0x8e, 0x00, 0x30, 0x10,
    0xa0, 0x21, 0x7d, 0xf7, 0x8e, 0x00, 0x30, 0x10,
    0xd0, 0x21, 0x17, 0x1c, 0xbd, 0x06, 0xf6, 0x10,	  //
    0xa0, 0x21, 0x10, 0x36, 0xbd, 0x06, 0xf6, 0x16,	  // exposure
    0xa0, 0x21, 0x76, 0x03, 0xbd, 0x06, 0xf6, 0x16,
    0xa0, 0x21, 0x11, 0x01, 0xbd, 0x06, 0xf6, 0x16,
    0xa0, 0x21, 0x00, 0x10, 0xbd, 0x06, 0xf6, 0x15,	  //gain
  //0xb0, 0x21, 0x2a, 0xc0, 0x3c, 0x06, 0xf6, 0x1d,//a0 1c,a0 1f,c0 3c frame rate ?line interval from ov6630
    0xb0, 0x21, 0x2a, 0xa0, 0x1f, 0x06, 0xf6, 0x10,
    0,
};

BOOL SenseOV7631(struct NepClassSonixcam *nch)
{
    ULONG pid;
    UBYTE *regs = initOv7630;
    UBYTE *sensinit = ov7630_sensor_init;

    WriteReg(nch, SXREG_CTRL, 0x01); // reset
    WriteReg(nch, SXREG_CTRL, 0x00); // release reset
    WriteReg(nch, SXREG_VID_CTRL_1, 0x28); // init

    nch->nch_I2CAddr = 0x21;

	pid = (ReadI2C(nch, 0x0a)<<8)|ReadI2C(nch, 0x0b);
    if(pid != 0x7631)
    {
        return FALSE;
    }
#if 0
    WriteReg(nch, 0x14, 0x00);
    WriteReg(nch, SXREG_VID_CTRL_1, 0x60);
    WriteReg(nch, SXREG_VID_CTRL_2, 0x0f);
    WriteReg(nch, SXREG_VID_CTRL_3, 0x50); // uncompressed, compressed 0x20

    WriteReg(nch, SXREG_WINDOW_LEFT, 1);
    WriteReg(nch, SXREG_WINDOW_TOP, 1);
    WriteReg(nch, SXREG_WINDOW_WIDTH, 640>>4);
    WriteReg(nch, SXREG_WINDOW_HEIGHT, 480>>4);

	WriteI2C(nch, 0x12, 0x8d);
	WriteI2C(nch, 0x12, 0x0d);
	WriteI2C(nch, 0x11, 0x00);
	WriteI2C(nch, 0x15, 0x35);
	WriteI2C(nch, 0x16, 0x03);
	WriteI2C(nch, 0x17, 0x1c);
	WriteI2C(nch, 0x18, 0xbd);
	WriteI2C(nch, 0x19, 0x06);
	WriteI2C(nch, 0x1a, 0xf6);
	WriteI2C(nch, 0x1b, 0x04);
	WriteI2C(nch, 0x20, 0x44);
	WriteI2C(nch, 0x23, 0xee);
	WriteI2C(nch, 0x26, 0xa0);
	WriteI2C(nch, 0x27, 0x9a);
	WriteI2C(nch, 0x28, 0x20);
	WriteI2C(nch, 0x29, 0x30);
	WriteI2C(nch, 0x2f, 0x3d);
	WriteI2C(nch, 0x30, 0x24);
	WriteI2C(nch, 0x32, 0x86);
	WriteI2C(nch, 0x60, 0xa9);
	WriteI2C(nch, 0x61, 0x42);
	WriteI2C(nch, 0x65, 0x00);
	WriteI2C(nch, 0x69, 0x38);
	WriteI2C(nch, 0x6f, 0x88);
	WriteI2C(nch, 0x70, 0x0b);
	WriteI2C(nch, 0x71, 0x00);
	WriteI2C(nch, 0x74, 0x21);
	WriteI2C(nch, 0x7d, 0xf7);

    WriteReg(nch, SXREG_CTRL, (ULONG) nch->nch_Reg[SXREG_CTRL]|0x04);
#else
    WriteReg(nch, SXREG_CTRL,       (ULONG) regs[SXREG_CTRL]);
    WriteReg(nch, SXREG_VID_CTRL_1, (ULONG) regs[SXREG_VID_CTRL_1]);
    WriteRegs(nch, SXREG_CTRL, 0x1f, &regs[SXREG_CTRL]);

    while(*sensinit)
    {
        WriteI2C_Raw(nch, sensinit);
        sensinit += 8;
    }

    WriteReg(nch, SXREG_WINDOW_LEFT, 2);
    WriteReg(nch, SXREG_WINDOW_TOP, 3);
    WriteReg(nch, SXREG_WINDOW_WIDTH, 640>>4);
    WriteReg(nch, SXREG_WINDOW_HEIGHT, 480>>4);
    WriteReg(nch, SXREG_VID_CTRL_1, 0x28);
    WriteReg(nch, SXREG_VID_CTRL_2, 0x1F);
    WriteReg(nch, SXREG_VID_CTRL_3, 0x50); // 0x33
#endif
    return(TRUE);
}

BOOL SenseOV7648(struct NepClassSonixcam *nch)
{
    ULONG pid;
    UBYTE *regs = initOv7630;
    UBYTE *sensinit = ov7630_sensor_init;

    WriteReg(nch, SXREG_CTRL, 0x01); // reset
    WriteReg(nch, SXREG_CTRL, 0x00); // release reset
    WriteReg(nch, SXREG_VID_CTRL_1, 0x28); // init

    nch->nch_I2CAddr = 0x21;

	pid = (ReadI2C(nch, 0x0a)<<8)|ReadI2C(nch, 0x0b);
    if(pid != 0x7648)
    {
        return FALSE;
    }
    WriteReg(nch, SXREG_CTRL,       (ULONG) regs[SXREG_CTRL]);
    WriteReg(nch, SXREG_VID_CTRL_1, (ULONG) regs[SXREG_VID_CTRL_1]);
    WriteRegs(nch, SXREG_CTRL, 0x1f, &regs[SXREG_CTRL]);

    while(*sensinit)
    {
        WriteI2C_Raw(nch, sensinit);
        sensinit += 8;
    }

    WriteReg(nch, SXREG_WINDOW_LEFT, 2);
    WriteReg(nch, SXREG_WINDOW_TOP, 3);
    WriteReg(nch, SXREG_WINDOW_WIDTH, 640>>4);
    WriteReg(nch, SXREG_WINDOW_HEIGHT, 480>>4);
    WriteReg(nch, SXREG_VID_CTRL_1, 0x28);
    WriteReg(nch, SXREG_VID_CTRL_2, 0x1F);
    WriteReg(nch, SXREG_VID_CTRL_3, 0x50); // 0x33
    return(TRUE);
}

BOOL SenseHV7131R(struct NepClassSonixcam *nch)
{
    ULONG val0;
    ULONG val1;

    WriteReg(nch, SXREG_CTRL, 0x01); // reset
    WriteReg(nch, SXREG_CTRL, 0x00); // release reset
    WriteReg(nch, SXREG_VID_CTRL_1, 0x28); // init

    //nch->nch_I2CAddr = 0x11;
    nch->nch_I2CAddr = 0x5d;

	val0 = ReadI2C(nch, 0x00);
	val1 = ReadI2C(nch, 0x01);
    Printf("PID=0x%02lx, Version=0x%02lx\n", val0, val1);
    return FALSE;
}

struct AutoBindData
{
    UWORD abd_VendID;
    UWORD abd_ProdID;
    UWORD abd_BridgeID;
};

struct AutoBindData CamBinds[] =
{
	{ 0x0c45, 0x6001, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6005, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6007, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6009, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6011, BRIDGE_SN9C102 },
	{ 0x0c45, 0x600d, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6019, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6024, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6025, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6028, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6029, BRIDGE_SN9C102 },
	{ 0x0c45, 0x602a, BRIDGE_SN9C102 },
	{ 0x0c45, 0x602b, BRIDGE_SN9C102 },
	{ 0x0c45, 0x602c, BRIDGE_SN9C102 },
	{ 0x0c45, 0x602d, BRIDGE_SN9C102 },
	{ 0x0c45, 0x602e, BRIDGE_SN9C102 },
	{ 0x0c45, 0x6030, BRIDGE_SN9C102 },
	{ 0x0c45, 0x603f, BRIDGE_SN9C102 },

	{ 0x0c45, 0x6080, BRIDGE_SN9C103 },
	{ 0x0c45, 0x6082, BRIDGE_SN9C103 },
	{ 0x0c45, 0x6083, BRIDGE_SN9C103 },
	{ 0x0c45, 0x6088, BRIDGE_SN9C103 },
	{ 0x0c45, 0x608a, BRIDGE_SN9C103 },
	{ 0x0c45, 0x608b, BRIDGE_SN9C103 },
	{ 0x0c45, 0x608c, BRIDGE_SN9C103 },
	{ 0x0c45, 0x608e, BRIDGE_SN9C103 },
	{ 0x0c45, 0x608f, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60a0, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60a2, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60a3, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60a8, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60aa, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60ab, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60ac, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60ae, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60af, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60b0, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60b2, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60b3, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60b8, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60ba, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60bb, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60bc, BRIDGE_SN9C103 },
	{ 0x0c45, 0x60be, BRIDGE_SN9C103 },

	{ 0x045e, 0x00f5, BRIDGE_SN9C105 },
	{ 0x045e, 0x00f7, BRIDGE_SN9C105 },
	{ 0x0471, 0x0327, BRIDGE_SN9C105 },
	{ 0x0471, 0x0328, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60c0, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60c2, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60c8, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60cc, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60ea, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60ec, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60ef, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60fa, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60fb, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60fc, BRIDGE_SN9C105 },
	{ 0x0c45, 0x60fe, BRIDGE_SN9C105 },

	{ 0x0458, 0x7025, BRIDGE_SN9C120 },
	{ 0x0458, 0x7034, BRIDGE_SN9C120 },
	{ 0x0c45, 0x6102, BRIDGE_SN9C120 },
	{ 0x0c45, 0x6108, BRIDGE_SN9C120 },
	{ 0x0c45, 0x610f, BRIDGE_SN9C120 },
	{ 0x0c45, 0x6130, BRIDGE_SN9C120 },
	{ 0x0c45, 0x6138, BRIDGE_SN9C120 },
	{ 0x0c45, 0x613a, BRIDGE_SN9C120 },
	{ 0x0c45, 0x613b, BRIDGE_SN9C120 },
	{ 0x0c45, 0x613c, BRIDGE_SN9C120 },
	{ 0x0c45, 0x613e, BRIDGE_SN9C120 },
    { 0x0000, 0x0000, 0 }
};


struct NepClassSonixcam * SetupSonixcam(void)
{
    struct NepClassSonixcam *nch;
    struct PsdDevice *pd = NULL;
    struct PsdAppBinding *pab;
    ULONG unit;
    struct AutoBindData *abd;

    if(ArgsArray[ARGS_UNIT])
    {
        unit = *((ULONG *) ArgsArray[ARGS_UNIT]);
    } else {
        unit = 0;
    }
    do
    {
        abd = CamBinds;
        do
        {
            pd = psdFindDevice(pd,
                               DA_VendorID, abd->abd_VendID,
                               DA_ProductID, abd->abd_ProdID,
                               TAG_END);
            if(!pd)
            {
                abd++;
                if(!abd->abd_VendID)
                {
                    break;
                }
            } else {
                if(!unit)
                {
                    break;
                }
                unit--;
            }
        } while(TRUE);

        if(!pd)
        {
            PutStr("No Sonixcam found!\n");
            return(NULL);
        }
        if((nch = psdAllocVec(sizeof(struct NepClassSonixcam))))
        {
            nch->nch_Device = pd;
            nch->nch_BridgeID= abd->abd_BridgeID;
            nch->nch_ReleaseHook.h_Entry = (APTR) releasehook;

            pab = psdClaimAppBinding(ABA_Device, pd,
                                     ABA_ReleaseHook, &nch->nch_ReleaseHook,
                                     ABA_UserData, nch,
                                     ABA_ForceRelease, TRUE,
                                     TAG_END);
            if(pab)
            {
                if(AllocSonixcam(nch))
                {
                    return(nch);
                } else {
                    PutStr("Couldn't allocate Sonixcam...\n");
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

void PasteText(struct SCImageHeader *scih, UBYTE *output)
{
    LONG x, y;
    LONG tarx, tary;
    LONG vw = scih->scih_ImgWidth;
    LONG vw3 = vw+vw+vw;
    UBYTE *op;
    WORD pix;
    if(tlength < vw-2)
    {
        tarx = (vw-tlength) >> 1;
    } else {
        tarx = 1;
    }
    tary = (scih->scih_ImgHeight-theight-theight);
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

struct NepClassSonixcam * AllocSonixcam(struct NepClassSonixcam *nch)
{
    BOOL foundsensor;
    nch->nch_Task = FindTask(NULL);

    nch->nch_Interface = psdFindInterface(nch->nch_Device, NULL,
                                          IFA_AlternateNum, 8,
                                          TAG_END);
    if(!nch->nch_Interface)
    {
        PutStr("No interfaces?\n");
        return(NULL);
    }

    psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                 IFA_InterfaceNum, &nch->nch_IfNum,
                 TAG_END);
    if((nch->nch_TaskMsgPort = CreateMsgPort()))
    {
        nch->nch_ImgDoneSig = AllocSignal(-1L);
        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
        {
            psdSetAltInterface(nch->nch_EP0Pipe, nch->nch_Interface);
            nch->nch_IsoEP = psdFindEndpoint(nch->nch_Interface, NULL,
                                             EA_IsIn, TRUE,
                                             EA_TransferType, USEAF_ISOCHRONOUS,
                                             TAG_END);
            if(nch->nch_IsoEP)
            {
                nch->nch_InReqHook.h_Entry = (HOOKFUNC) nInReqHook;
                nch->nch_InReqHook.h_Data = nch;
                nch->nch_InDoneHook.h_Entry = (HOOKFUNC) nInDoneHook;
                nch->nch_InDoneHook.h_Data = nch;
                nch->nch_RTIso = psdAllocRTIsoHandler(nch->nch_IsoEP,
                                                      RTA_InRequestHook, &nch->nch_InReqHook,
                                                      RTA_InDoneHook, &nch->nch_InDoneHook,
                                                      TAG_END);
                if(nch->nch_RTIso)
                {
                    foundsensor = SenseOV7631(nch);
                    if(!foundsensor)
                    {
                        foundsensor = SenseHV7131R(nch);
                    }

                    if(foundsensor)
                    {
                        switch(nch->nch_BridgeID)
                        {
                            case BRIDGE_SN9C102:
                                nch->nch_HeaderSize = 12;
                                break;

                            case BRIDGE_SN9C103:
                                nch->nch_HeaderSize = 18;
                                break;

                            case BRIDGE_SN9C105:
                            case BRIDGE_SN9C120:
                                nch->nch_HeaderSize = 62;
                                break;
                        }

                        nch->nch_RawBufSize = 320*240 + nch->nch_HeaderSize;
                        nch->nch_RawBuf[0] = psdAllocVec(nch->nch_RawBufSize * 3);
                        if(nch->nch_RawBuf[0])
                        {
                            nch->nch_RawBuf[1] = nch->nch_RawBuf[0] + nch->nch_RawBufSize;
                            nch->nch_RawBuf[2] = nch->nch_RawBuf[1] + nch->nch_RawBufSize;
                            nch->nch_ImgBufSize = 320*240*3;
                            nch->nch_ImgBuf = psdAllocVec(nch->nch_ImgBufSize);
                            if(nch->nch_ImgBuf)
                            {
                                return(nch);
                            } else {
                                Printf("Couldn't allocate %ld bytes of memory.\n", nch->nch_ImgBufSize);
                            }
                            psdFreeVec(nch->nch_RawBuf[0]);
                        } else {
                            Printf("Couldn't allocate %ld bytes of memory.\n", nch->nch_RawBufSize * 3);
                        }
                    } else {
                        PutStr("Failed to init sensor\n");
                    }
                    psdFreeRTIsoHandler(nch->nch_RTIso);
                } else {
                    PutStr("Couldn't allocate RT Iso Handler.\n");
                }
            } else {
                PutStr("No iso endpoint?\n");
            }
            psdFreePipe(nch->nch_EP0Pipe);
        } else {
            PutStr("Couldn't allocate default pipe\n");
        }
        FreeSignal(nch->nch_ImgDoneSig);
        DeleteMsgPort(nch->nch_TaskMsgPort);
    }
    return(NULL);
}

void FreeSonixcam(struct NepClassSonixcam *nch)
{
    APTR pab;
    psdFreeVec(nch->nch_ImgBuf);
    psdFreeVec(nch->nch_RawBuf[0]);
    psdFreeRTIsoHandler(nch->nch_RTIso);
    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_Binding, &pab,
                TAG_END);
    psdReleaseAppBinding(pab);
    psdFreePipe(nch->nch_EP0Pipe);
    FreeSignal(nch->nch_ImgDoneSig);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    psdFreeVec(nch);
}

/**************************************************************************/

void bayer_unshuffle(struct SCImageHeader *scih, UBYTE *raw, UBYTE *output)
{
    ULONG x, y;
    ULONG vw = scih->scih_ImgWidth;
    ULONG vh = scih->scih_ImgHeight;
    ULONG w3 = vw + vw +vw;
    ULONG w12 = vw>>1;
    UBYTE *oline;

/* Byte Order. Each cell is one byte.
   start + 0:  B00 G01 B02 G03
   start + 4:  G10 R11 G12 R13
   start + 8:  B20 G21 B22 G23
   start + 12: G30 R31 G32 R33 */

/* Byte Order. Each cell is one byte.
   start + 0:  G00 B01 G02 B03
   start + 4:  R10 G11 R12 G13
   start + 8:  G20 B21 G22 B23
   start + 12: R30 G31 R32 G33 */

    // RGB
    for(y = 0; y < vh; y++)
    {
        oline = output + 1 - (y & 1); // start with green on even, red on odd
        x = w12;
        do
        {
            *oline = *raw++; // green on even, red on odd
            oline += 4;
            *oline = *raw++; // blue on even, green on odd
            oline += 2;
        } while(--x);
        output += w3;
    }
}

void bayer_demosaic(struct SCImageHeader *scih, UBYTE *output)
{
    LONG x, y;
    UBYTE *op;

#define vw3 (320*3)

    op = output + 3 + vw3;
    y = (240 - 2)>>1;
    do
    {
        x = (320 - 2)>>1;
        do
        {
             /* 11 green. red lr, blue tb */
             *op = (((UWORD) op[-3]) + ((UWORD) op[3])) >> 1; /* Set red */
             op[2] = (((UWORD) op[2-vw3]) + ((UWORD) op[2+vw3]) + 1) >> 1; /* Set blue */
             op += 3;
             /* 10 red. green lrtb, blue diagonals */
             op[1] = (((UWORD) op[-2]) + ((UWORD) op[4]) +
                     ((UWORD) op[1-vw3]) + ((UWORD) op[1+vw3]) + 2) >> 2; /* Set green */
             op[2] = (((UWORD) op[-1-vw3]) + ((UWORD) op[5-vw3]) +
                     ((UWORD) op[-1+vw3]) + ((UWORD) op[5+vw3]) + 2) >> 2; /* Set blue */
             op += 3;
        } while(--x);
        op += 6;
        x = (320 - 2)>>1;
        do
        {
             /* 01 blue. green lrtb, red diagonals */
             op[1] = (((UWORD) op[-2]) + ((UWORD) op[4]) +
                     ((UWORD) op[1-vw3]) + ((UWORD) op[1+vw3]) + 2) >> 2; /* Set green */
             *op = (((UWORD) op[-3-vw3]) + ((UWORD) op[3-vw3]) +
                   ((UWORD) op[-3+vw3]) + ((UWORD) op[3+vw3]) + 2) >> 2; /* Set red */
             op += 3;
             /* 00 green. blue lr, red tb */
             op[2] = (((UWORD) op[-1]) + ((UWORD) op[5]) + 1) >> 1; /* Set blue */
             *op = (((UWORD) op[-vw3]) + ((UWORD) op[vw3]) + 1) >> 1; /* Set red */
             op += 3;
        } while(--x);
        op += 6;
    } while(--y);
    // do border
    x = 320;
    op = output;
    do
    {
        *op++ = 0;
        *op++ = 0;
        *op++ = 0;
    } while(--x);

    y = 238;
    do
    {
        *op++ = 0;
        *op++ = 0;
        *op = 0;
        op += vw3-5;
        *op++ = 0;
        *op++ = 0;
        *op++ = 0;
    } while(--y);

    x = 320;
    do
    {
        *op++ = 0;
        *op++ = 0;
        *op++ = 0;
    } while(--x);
#undef vw3
}

void gammacorrection(struct SCImageHeader *scih, UBYTE *output)
{
    ULONG cnt = scih->scih_ImgWidth*scih->scih_ImgHeight;
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

void sharpen5x5(struct SCImageHeader *scih, UBYTE *input, UBYTE *output)
{
    LONG x, y;
    LONG vw = scih->scih_ImgWidth;
    LONG vw3 = vw+vw+vw;
    LONG vw6 = vw*6;
    LONG vh = scih->scih_ImgHeight;
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

APTR GetVideoSnap(struct NepClassSonixcam *nch, struct SCImageHeader *scih)
{
    LONG ioerr;
    UBYTE *newimgbuf;
    ULONG sigs;
    ULONG lastdone;

    scih->scih_ImgWidth = 320;
    scih->scih_ImgHeight = 240;
    scih->scih_ImgSize = scih->scih_ImgWidth * scih->scih_ImgHeight * 3;

    nch->nch_BufState[2] = nch->nch_BufState[1] = nch->nch_BufState[0] = BUF_EMPTY;

    nch->nch_CurrIsoBuf = nch->nch_RawBuf[0];
    nch->nch_IsoBufNum = 0;
    nch->nch_IsoBufPos = 0;

    ioerr = psdStartRTIso(nch->nch_RTIso);
    if(ioerr)
    {
        Printf("Error starting RT Iso errorcode=%ld\n", ioerr);
        return(NULL);
    }

    do
    {
        sigs = Wait((1UL<<nch->nch_ImgDoneSig)|SIGBREAKF_CTRL_C);
        lastdone = nch->nch_LastDoneNum;
        if(nch->nch_BufState[lastdone] == BUF_READY)
        {
            break;
        }
    } while(!(sigs & SIGBREAKF_CTRL_C));

    ioerr = psdStopRTIso(nch->nch_RTIso);

    if(sigs & SIGBREAKF_CTRL_C)
    {
        return(NULL);
    }

    nch->nch_BufState[lastdone] = BUF_BUSY;
    bayer_unshuffle(scih, nch->nch_RawBuf[lastdone] + nch->nch_HeaderSize, nch->nch_ImgBuf);
    nch->nch_BufState[lastdone] = BUF_EMPTY;
    bayer_demosaic(scih, nch->nch_ImgBuf);

    if(ArgsArray[ARGS_SHARPEN])
    {
        newimgbuf = psdAllocVec(nch->nch_ImgBufSize);
        if(newimgbuf)
        {
            sharpen5x5(scih, nch->nch_ImgBuf, newimgbuf);
            psdFreeVec(nch->nch_ImgBuf);
            nch->nch_ImgBuf = newimgbuf;
        }
    }
    if(ArgsArray[ARGS_GAMMA])
    {
        gammacorrection(scih, nch->nch_ImgBuf);
    }
    if(ArgsArray[ARGS_TEXT])
    {
        PasteText(scih, nch->nch_ImgBuf);
    }
    return(nch->nch_ImgBuf);
}

int main(int argc, char *argv[])
{
    struct NepClassSonixcam *nch;
    struct SCImageHeader scih;
    BPTR outfile;
    UBYTE *imgbuf;
    ULONG sigs;
    char buf[256];
    ULONG imgcount;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        PutStr("Wrong arguments!\n");
        return(RETURN_FAIL);
    }
    ps = OpenLibrary("poseidon.library", 4);
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
    if(!(nch = SetupSonixcam()))
    {
        FreeText();
        FreeArgs(ArgsHook);
        CloseLibrary(ps);
        return(RETURN_ERROR);
    }
    CreateGammaTab();
    imgcount = 0;
    do
    {
         /*PutStr("Waiting for CTRL-C before downloading.\n"
                "This is your chance to remove the cam to check if the machine crashes.\n");
         Wait(SIGBREAKF_CTRL_C); */
         imgbuf = GetVideoSnap(nch, &scih);
         if(imgbuf)
         {
             psdSafeRawDoFmt(buf, 256, (STRPTR) ArgsArray[ARGS_TO], imgcount);
             outfile = Open(buf, MODE_NEWFILE);
             if(outfile)
             {
                 FPrintf(outfile, "P6\n%ld %ld\n255\n", scih.scih_ImgWidth, scih.scih_ImgHeight);
                 Flush(outfile);
                 Write(outfile, imgbuf, scih.scih_ImgSize);
                 Close(outfile);
                 Printf("Wrote image into '%s'.\n", buf);
                 imgcount++;
             } else {
                 Printf("Could not open file '%s' for writing!\n", buf);
             }
         } else {
             break;
         }
         /*PutStr("Finished. Waiting for another CTRL-C.\n");*/
         sigs = SetSignal(0, 0);
         if(sigs & SIGBREAKF_CTRL_C)
         {
             break;
         }
         if(ArgsArray[ARGS_INTERVAL])
         {
             if(ArgsArray[ARGS_UPTO])
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
                 if(imgcount > *((ULONG *) ArgsArray[ARGS_UPTO]))
                 {
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
    FreeSonixcam(nch);
    FreeArgs(ArgsHook);
    CloseLibrary(ps);
    return(RETURN_OK);
}
