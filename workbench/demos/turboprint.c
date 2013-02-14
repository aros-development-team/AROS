/***********************************
 * C-Source
 * Demonstration of printing 24bit
 * graphics bitmap with TurboPrint 
 * Professional 3.x/4.x
 * December 1995
 ***********************************
 * IrseeSoft
 * Meinrad-Spiess-Platz 2
 * D-87660 Irsee
 * Germany
 * Tel. (49) 8341 74327
 * Fax  (49) 8341 12042
 * Email: IrseeSoft@t-online.de
 ***********************************/

#include <stdio.h>

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/devices.h>
#include <devices/printer.h>
#include <devices/prtbase.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <libraries/cybergraphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

// some new Includes
#include <turboprint.h>


#define Width 256
#define Height 256

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;


union printerIO
{
	struct IOStdReq	ios;
	struct IODRPReq	iodrp;
	struct IOPrtCmdReq iopc;
};



// this short demo programm opens an invisible 24bit RastPort and
// dumps it through TurboPrint to the printer
// You see: it works almost like the normal graphics dump

int main(int argc, char **argv)
{
struct MsgPort *PrinterMP;
union printerIO *PIO;
struct PrinterData *PD;
struct TPExtIODRP ExtIoDrp;

struct RastPort MyRastPort;
struct BitMap MyBitMap;
UWORD x,y;
UBYTE *MemPoint;
BOOL TP_Installed;

UWORD TPVersion;
char* TPIdString;

if ((IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library",37)))
if ((GfxBase = (struct GfxBase*)OpenLibrary("graphics.library",37)))
	{

// create a RastPort Width * Height 24-Bit

	InitRastPort(&MyRastPort);
	MyRastPort.BitMap = &MyBitMap;
		// we need only one BitPlane, because it's chunky format
	InitBitMap(&MyBitMap,1,Width,Height);

	// now set the right BytesPerRow
	// we could have set it as 3*Width in InitBitMap but
	// InitBitMap sometimes calculates wrong BytesPerRow
	// so we do it on our own

	MyBitMap.BytesPerRow = 3*Width;


	// allocate a demo bitmap

	if ((MyBitMap.Planes[0]= (APTR)AllocMem(3*Width*Height,0)))
		{
		MemPoint = (UBYTE*)MyBitMap.Planes[0];

	// and fill it with some colored stuff

		for (y=0;y<256;y++)
			for (x=0;x<256;x++)
			{
				// fill it with some nice colors
				*MemPoint++ = 255-((x+y)>>1);		// red
				*MemPoint++ = (x>127)? 2*(x-128):255-2*x;	// green
				*MemPoint++ = (y>127)? ((x<127)? 255-((x+y)>>1):2*(y-127)):0;	// blue
			}				


	// now create a MessagePort and Open the printer.device as usual

		if ((PrinterMP = (struct MsgPort*)CreateMsgPort()))
			{
			if ((PIO = (union printerIO *)CreateExtIO(PrinterMP,sizeof(union printerIO))))
				{
				// open printer.device, if TurboPrint is installed, we automatically access the
				// turboprint printer.device
				if (!(OpenDevice("printer.device",0,(struct IORequest *)PIO,0)))
					{

					// before printing we need to know if TurboPrint is installed
					// therefore check the MatchWord in the "PrinterData" structure
					// element "pd_OldStack"

					PD = (struct PrinterData *)PIO->iodrp.io_Device;
					TP_Installed = ( ((ULONG *)(PD->pd_OldStk))[2] == TPMATCHWORD);

					TPVersion = PIO->iodrp.io_Device->dd_Library.lib_Version;

					TPIdString = (char*)(PIO->iodrp.io_Device->dd_Library.lib_IdString);

					// you could now check chars 25-27 for version and revision
					printf("Device-Version:%d\nTPIdString:%s\n",TPVersion,&TPIdString[1]);


					if (TP_Installed && TPVersion >= 39)
						{					
					// fill in IORequest as normal
						PIO->iodrp.io_Command = PRD_TPEXTDUMPRPORT;
						PIO->iodrp.io_RastPort = &MyRastPort; 
						PIO->iodrp.io_ColorMap = 0;
						PIO->iodrp.io_SrcX = 0;
						PIO->iodrp.io_SrcY = 0;
						PIO->iodrp.io_SrcWidth = Width;
						PIO->iodrp.io_SrcHeight = Height;
						// all Special Flags are possible, here we do not need any
						PIO->iodrp.io_Special = 0;


						// new: io.Modes must point to a new Structure (ExtIoDrp)
						PIO->iodrp.io_Modes = (IPTR)&ExtIoDrp;
					
					// fill in the new structure
						ExtIoDrp.PixAspX = 1;	// for the correct aspect ratio
						ExtIoDrp.PixAspY = 1;   // normally the values of the monitor-structure
														// TicksX and TicksY
					// our bitmap is 3 bytes format RGB (24 bits)
						ExtIoDrp.Mode = TPFMT_RGB24;

					// send IORequest as usual to the printer.device
					// Turboprint behaves exactly like the old printer.device
					// so SendIO etc. are also possible					
						DoIO((struct IORequest *)PIO);
						}
					// if TurboPrint is not installed: Error-Message
					// also possible: load TurboPrint:TurboPrefs with the -q flag, so 
					// you can install it yourself
						else printf("Error: TurboPrint Pro 3 (or higher) not installed\n");	

					CloseDevice((struct IORequest *)PIO);
					}
				DeleteExtIO((struct IORequest *)PIO);
				}
			DeleteMsgPort(PrinterMP);
			}
		FreeMem(MyBitMap.Planes[0],MyBitMap.BytesPerRow*MyBitMap.Rows);
		}
	}	
if (GfxBase) CloseLibrary((struct Library*)GfxBase);
if (IntuitionBase) CloseLibrary((struct Library*)IntuitionBase);

   return 0;
}
