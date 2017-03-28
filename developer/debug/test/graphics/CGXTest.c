/*
 * This source was ocassionally found by Google here:
 * http://megaburken.net/~patrik/
 *
 * The code is unmodified (except fixed #include file names in order to compile
 * on modern systems)
 *
 * I hope the original author is not against spreading it, especially taking
 * into account its experimental nature.
 *					Pavel Fedin <pavel_fedin@mail.ru>
 */

#include <stdio.h>
#include <stdlib.h>

#include <exec/exec.h>       
#include <graphics/gfxbase.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <proto/cybergraphics.h>

#define WIDTH   320
#define HEIGHT  240
#define DEPTH   8


struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct IntuitionBase *IntuitionBase;

int main(int argc, char *argv[])
{
	struct Screen *myScreen;
	struct Window *myWindow;
	struct RastPort myRastPort;
	struct BitMap *myBitMap;

	APTR bitMapHandle;

	ULONG result;

	ULONG width;
	ULONG height;
	ULONG depth;

	ULONG bm_width;
	ULONG bm_depth;
	ULONG bm_height;
	ULONG bm_pixfmt;
	ULONG bm_bytesperpix;
	ULONG bm_bytesperrow;
	IPTR  bm_baseaddress;
	IPTR  bm_endaddress;
	volatile ULONG *bm_curraddress;

	if(argc == 1)
	{
		width = WIDTH;
		height = HEIGHT;
		depth = DEPTH;
	}
	else if(argc == 4)
	{
		width = atoi(argv[1]);
		height = atoi(argv[2]);
		depth = atoi(argv[3]);
	}
	else
	{
		printf("Wrong number of arguments!\n\n");
		printf("Usage: CGXTest width height depth\n\n");
		return 1;
	}


	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39L);
	if(!GfxBase)
	{
		printf("Couldnt open graphics.library 40.\n");
		return 2;
	}
	CyberGfxBase = OpenLibrary("cybergraphics.library", 41L);
	if(!CyberGfxBase)
	{
		printf("Couldnt open cybergraphics.library 41.\n");
		CloseLibrary((struct Library *)GfxBase);
		return 3;
	}
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39L);
	if(!IntuitionBase)
	{
		printf("Couldnt open intuition.library 39.\n");
		CloseLibrary(CyberGfxBase);
		CloseLibrary((struct Library *)GfxBase);
		return 4;
	}

	myScreen = LockPubScreen(NULL);
	if(myScreen != NULL)
	{
		UnlockPubScreen(NULL, myScreen);
	}
	else
	{
		printf("Couldnt get screen data.\n");
		CloseLibrary((struct Library*)IntuitionBase);
		CloseLibrary(CyberGfxBase);
		CloseLibrary((struct Library *)GfxBase);

		return 5;
	}

	myBitMap = AllocBitMap(width, height, depth, BMF_MINPLANES | BMF_DISPLAYABLE, myScreen->RastPort.BitMap);
	if(!myBitMap)
	{
		printf("Couldnt allocate bitmap.\n");
		CloseLibrary(CyberGfxBase);
		CloseLibrary((struct Library *)GfxBase);
		CloseLibrary((struct Library *)IntuitionBase);
		return 6;
	}

	//Creates the RastPort used for blitting
	InitRastPort(&myRastPort);
	myRastPort.BitMap = myBitMap;

	myWindow = OpenWindowTags(NULL, //WA_Left,        100,
									//WA_Top,         100,
									WA_InnerWidth,  width,
									WA_InnerHeight, height,
									WA_ScreenTitle, "Watch out for that trashing! ;)",
									TAG_END);
	if(myWindow == NULL)
	{
		printf("Couldnt open a new window.\n");
		FreeBitMap(myBitMap);

		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary(CyberGfxBase);
		CloseLibrary((struct Library *)GfxBase);
		return 5;
	}


	printf("Allocated BitMap data:\n");

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_ISCYBERGFX);
	if(result)
		printf("It is a CyberGraphX bitmap!\n");
	else
		printf("It is not a CyberGraphX bitmap!\n");

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_ISLINEARMEM);
	if(result)
		printf("It supports linear memory access!\n");
	else
		printf("It does not support linear memory access!\n");

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_WIDTH);
	printf("Width: %d\n", (int)result);

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_HEIGHT);
	printf("Height: %d\n", (int)result);

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_DEPTH);
	printf("Depth: %d\n", (int)result);

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_PIXFMT);
	printf("Pixel format: ");
	switch(result)
	{
		case PIXFMT_LUT8:
			printf("LUT8");
			break;
		case PIXFMT_RGB15:
			printf("RGB15");
			break;
		case PIXFMT_BGR15:
			printf("BGR15");
			break;
		case PIXFMT_RGB15PC:
			printf("RGB15PC");
			break;
		case PIXFMT_BGR15PC:
			printf("BGR15PC");
			break;
		case PIXFMT_RGB16:
			printf("RGB16");
			break;
		case PIXFMT_BGR16:
			printf("BGR16");
			break;
		case PIXFMT_RGB16PC:
			printf("RGB16PC");
			break;
		case PIXFMT_BGR16PC:
			printf("BGR16PC");
			break;
		case PIXFMT_RGB24:
			printf("RGB24");
			break;
		case PIXFMT_BGR24:
			printf("BGR24");
			break;
		case PIXFMT_ARGB32:
			printf("ARGB32");
			break;
		case PIXFMT_BGRA32:
			printf("BGRA32");
			break;
		case PIXFMT_RGBA32:
			printf("RGBA32");
			break;
		default:
			printf("UNDEFINED");
	}
	printf("\n");

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_BPPIX);
	printf("Bytes per pixel: %d\n", (int)result);

	result=GetCyberMapAttr(myBitMap, CYBRMATTR_XMOD);
	printf("Bytes per row: %d\n\n", (int)result);

	printf("Locking BitMap and writing to it...");
	bitMapHandle = LockBitMapTags(myBitMap,
									LBMI_WIDTH,         &bm_width,
									LBMI_HEIGHT,        &bm_height,
									LBMI_DEPTH,         &bm_depth,
									LBMI_PIXFMT,        &bm_pixfmt,
									LBMI_BYTESPERPIX,   &bm_bytesperpix,
									LBMI_BYTESPERROW,   &bm_bytesperrow,
									LBMI_BASEADDRESS,   &bm_baseaddress,
									TAG_END);

	if(bitMapHandle)
	{
		//Trashing?
		bm_endaddress = bm_baseaddress + bm_bytesperrow * bm_height;
		for(bm_curraddress = (ULONG *)bm_baseaddress; bm_curraddress < (ULONG *)bm_endaddress; bm_curraddress++)
			*bm_curraddress = 0x102B5FF1;

		UnLockBitMap(bitMapHandle);

		printf("...done.\n\n");


		printf("Locked BitMap data:\n");
		printf("Base address: %p\n", (APTR)bm_baseaddress);
		printf("Width: %d\n", (int)bm_width);
		printf("Height: %d\n", (int)bm_height);
		printf("Depth: %d\n", (int)bm_depth);
		printf("Pixel format: ");
		switch(bm_pixfmt)
		{
			case PIXFMT_LUT8:
				printf("LUT8");
				break;
			case PIXFMT_RGB15:
				printf("RGB15");
				break;
			case PIXFMT_BGR15:
				printf("BGR15");
				break;
			case PIXFMT_RGB15PC:
				printf("RGB15PC");
				break;
			case PIXFMT_BGR15PC:
				printf("BGR15PC");
				break;
			case PIXFMT_RGB16:
				printf("RGB16");
				break;
			case PIXFMT_BGR16:
				printf("BGR16");
				break;
			case PIXFMT_RGB16PC:
				printf("RGB16PC");
				break;
			case PIXFMT_BGR16PC:
				printf("BGR16PC");
				break;
			case PIXFMT_RGB24:
				printf("RGB24");
				break;
			case PIXFMT_BGR24:
				printf("BGR24");
				break;
			case PIXFMT_ARGB32:
				printf("ARGB32");
				break;
			case PIXFMT_BGRA32:
				printf("BGRA32");
				break;
			case PIXFMT_RGBA32:
				printf("RGBA32");
				break;
			default:
				printf("UNDEFINED");
		}
		printf("\n");
		printf("Bytes per pixel: %d\n", (int)bm_bytesperpix);
		printf("Bytes per row: %d\n", (int)bm_bytesperrow);
		printf("\n");

		printf("Blitting BitMap to window...");
		ClipBlit(&myRastPort, 0, 0, myWindow->RPort, myWindow->BorderLeft, myWindow->BorderTop, bm_width, bm_height, 0xC0);
		printf("...done.\n");
	}
	else
		printf("failed, couldn't lock bitmap.\n");

	printf("\nPress enter to close the window.");
	result = (ULONG)getchar();

	CloseWindow(myWindow);

	FreeBitMap(myBitMap);

	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary(CyberGfxBase);
	CloseLibrary((struct Library *)GfxBase);

	printf("\nExiting.\n");
	return 0;
}
