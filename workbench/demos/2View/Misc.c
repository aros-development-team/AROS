/* Modified version for AROS - The Amiga Research OS
** $Id$
*/

#include <stdio.h>
#include <stdarg.h>

#include <exec/types.h>
#include <dos/rdargs.h>
/* #include <devices/printer.h>
#include <devices/prtgfx.h> */
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <graphics/gfxmacros.h>
#include <graphics/copper.h>
#include <hardware/custom.h>

#include "iff.h"
#include "2View.h"
#include "arexx.h"

/*Prototypes*/
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/alib.h>

BPTR StdErr=NULL;   /*'Standard error' for AmigaDOS IO functions*/

/*These are defined in 2View.c, but this file needs to reference them*/

extern struct NewScreen newScreen;
extern struct NewWindow newWindow;

extern struct IFFHandle *iff;	/*IFF handle*/
extern BPTR pL; 		/*Playlist file pointer*/
extern BOOL masking,print,toFront,printPics;

/*A true here indicates the current ILBM file is compressed*/
extern BYTE Compression;

extern char trashBuf[512];	 /* A place to dump mask information */

extern struct Screen *screen;
extern struct Window *window;

/*The previous screen and window*/
extern struct Window *prevWindow;
extern struct Screen *prevScreen;

/*Libraries we'll need*/
extern struct Library *IFFParseBase;
/* extern struct Library *IntuitionBase; */
extern struct GfxBase *GfxBase;

/*Provided by the compiler*/
/* extern struct Library *SysBase;
extern struct Library *DOSBase; */

extern BOOL cycle;
extern UBYTE numColors;
UWORD colorTable[32];
extern UWORD destMap[32];

extern struct Custom custom;

/*Convert the CMAP information into a color map, then set the colors*/
/*of the screen according to that colormap*/
void setScreenColors(struct Screen *scr, UBYTE *colorMap, UBYTE depth,
		     UWORD *destColorMap,UBYTE *colors)
{
   int i,numColors;

   numColors=1<<depth;	/*Get the number of colors (generally 2^depth)*/

   if(newScreen.ViewModes & HAM)    /*There are, of course, 2 exceptions*/
      numColors=16;
   if(newScreen.ViewModes & EXTRA_HALFBRITE)
      numColors=32;

      /*For each color, convert it from CMAP to Amiga form and*/
      /*store it (both in an unchanging table (colorTable) and in a "work" */
      /*table (destColorMap), where the values are free to change during */
      /*color cycling, etc.*/
   for(i=0;i<numColors;i++)
      colorTable[i]=destColorMap[i]=
	    (15 & (colorMap[i*3])>>4)<<8 | (15 & (colorMap[i*3+1])>>4)<<4 |
	       (15 & (colorMap[i*3+2]>>4));

      /*Store the color table*/
   LoadRGB4(&(scr->ViewPort),destColorMap,numColors);

      /*Return the number of colors*/
   *colors=numColors;

   return;
}

#define GetWord(ptr)        ((ptr[0] << 8) + ptr[1])

/*Make a newScreen structure using the BMHD chunk*/
void getBMHD(UBYTE * ptr)
{
    struct BitMapHeader bmhd;

    bmhd.w = GetWord(ptr); ptr += 2;
    bmhd.h = GetWord(ptr); ptr += 2;
    bmhd.x = GetWord(ptr); ptr += 2;
    bmhd.y = GetWord(ptr); ptr += 2;
    bmhd.nplanes = *ptr++;
    bmhd.Masking = *ptr++;
    bmhd.Compression = *ptr++;
    ptr++; /* pad */
    bmhd.TransparentColor = *ptr++;
    bmhd.XAspect = *ptr++;
    bmhd.YAspect = *ptr++;
    bmhd.PageWidth = GetWord(ptr); ptr += 2;
    bmhd.PageHeight = GetWord(ptr);

				  /*Define the screen as hires if*/
   if(bmhd.PageWidth > 400 && bmhd.PageWidth <=704 && bmhd.nplanes < 5)
      newScreen.ViewModes|=HIRES;      /*wider than 400 pixels and not */
				       /*deeper than 4 bitplanes*/

   if(bmhd.PageHeight > 300 && bmhd.PageHeight <=512)  /*Define the screen as interlaced*/
      newScreen.ViewModes|=LACE;       /*if the height is > 300 pixels*/

   newScreen.Width=bmhd.w;	      /*Store the rest of the values*/
   newScreen.Height=bmhd.h;

   newScreen.LeftEdge=bmhd.x;
   newScreen.TopEdge=bmhd.y;
   newScreen.Depth=bmhd.nplanes;

   masking=(bmhd.Masking == 1);

   Compression=(bmhd.Compression!=0);   /*Compression flag.  Store for*/
				    /*later use*/
   return;
}

/*Data structures for ReadArgs()*/
struct RDArgs ra=
{
   {NULL,0,0},
   0L,
   trashBuf,
   512,
   "FILE/A/M,SECS=SECONDS/K/N,TICKS/K/N,LOOP/S,FROM/K,PRINT/S"
};

/*Parse the argument list, using ReadArgs()*/
void ParseArgs(IPTR *args)
{
   ReadArgs("FILE/A/M,SECS=SECONDS/K/N,TICKS/K/N,LOOP/S,FROM/K,PRINT/S",
	    args,&ra);
   return;
}

/*Check to see which mouse buttons have been pressed*/
ButtonTypes checkButton(void)
{
   struct IntuiMessage *mesg;
   ButtonTypes Button=none;
   static int justActivated=FALSE;

      /*This function disregards a select (left) mouse button click*/
      /*if the window's just been activated.  This is so that a user*/
      /*can click on another window, then make this one active again,*/
      /*without advancing to the next picture*/

	 /*While there are messages to be read...*/
   while((mesg=(struct IntuiMessage *)GetMsg(prevWindow->UserPort))!=NULL)
   {
	 /*Interpret them*/
      switch(mesg->Class)
      {
	 case IDCMP_ACTIVEWINDOW:   /*Set the appropriate flag if the window*/
	    justActivated=TRUE;  /*was just activated*/
	    break;
	 case IDCMP_VANILLAKEY:
	    switch(mesg->Code)
	    {
	       case 16:       /*CTRL-P - Print (if this picture hasn't been*/
		  if(print)   /*printed;  this is designed in case the user*/
		  {	      /*holds down CTRL-P: we don't want 10-20     */
			      /*print requests to get queued up 	   */
		     dumpRastPort(&(prevScreen->RastPort),
				  &(prevScreen->ViewPort));
		     print=FALSE;
		  }
		  break;
	       case 4:	      /*CTRL-D - Abort everything*/
		  Button=menu;
		  break;
	       case 3:
		  Button=select; /*CTRL-C - Advance to next picture*/
		  break;
	       case 9:	      /*TAB:  Switch color cycling on/off*/
		  toggleCycling();
		  break;

	    }
	    break;
	 case IDCMP_MOUSEBUTTONS:   /*Interpret a button click*/
	    if(mesg->Code==SELECTDOWN) /*If the left button was pushed,*/
            {
	       if(justActivated)       /*and not so as to activate the*/
	       {		       /*window, advance to the next*/
		  justActivated=FALSE;	     /*screen*/
		  break;
	       }
	       else
		  Button=select;
	    } else if(mesg->Code == MENUDOWN)  /*If the right button was*/
	       Button=menu;		     /*pushed, we'll want to*/
	    break;			     /*abort*/
      }
      ReplyMsg((struct Message *)mesg);      /*Reply to the message*/
   }
   return(Button);                           /*Return the results*/
}

/*Toggle color cycling on and off*/
void toggleCycling(void)
{
   int c;

   cycle=(cycle) ? FALSE : TRUE;    /*Toggle the color cycling lag*/
   LoadRGB4(&(prevScreen->ViewPort),colorTable,numColors);
   for(c=0;c<numColors;c++)
      destMap[c]=colorTable[c];
}


struct EasyStruct erError2Line =
{
   sizeof(struct EasyStruct),
   0,
   "Program error:  2View",
   "%s\n%s\n%s",
   "Ok"
};

struct EasyStruct erError1Line =
{
   sizeof(struct EasyStruct),
   0,
   "Program error:  2View",
   "%s",
   "Ok"
};

/*This prints an error to the console, if we were run from the CLI*/
/*This is done instead of using Output() so as to get around any redirection*/
/*that may be in place (just like the standard C stderr)*/
/*If we can't open a StdErr or 2View was run from Workbench, a requester */
/*is put up*/
void printError(char *fmt,...)
{
   char error[1024];
   va_list args;

   va_start(args,fmt);

   vsnprintf (error, sizeof(error), fmt, args);

   va_end (args);

   if(StdErr==NULL)
      StdErr=Open("CONSOLE:",MODE_OLDFILE);

   /* If we can't open StdErr, or Output()==NULL (meaning we're running */
   /* Workbench), put up a requester */
   if(StdErr==NULL || Output()==NULL)
   {
      EasyRequest(NULL,&erError1Line,NULL,(IPTR)error,(IPTR)"Exiting...");
   }
   else
   {
      FPuts(StdErr,error);
      FPuts(StdErr,"\nExiting\n");
   }
   return;
}

/*Free allocated resources in anticipation of quitting*/
void cleanup()
{
#if 0
      /*Close the ARexx port*/
   dnRexxPort();
#endif

      /*Close the standard-error file if opened*/
   if(StdErr!=NULL)
      Close(StdErr);

      /*Close a previous screen and window, if open*/
   if(prevWindow!=NULL)
      CloseWindow(prevWindow);
   if(prevScreen!=NULL)
      CloseScreen(prevScreen);

      /*Close a current screen and window, if open*/
   if(window!=NULL)
      CloseWindow(window);
   if(screen!=NULL)
      CloseScreen(screen);

   if(iff!=NULL)
      FreeIFF(iff);

   if(pL!=NULL)
      Close(pL);

   if(IFFParseBase!=NULL)
      CloseLibrary(IFFParseBase);

   if(IntuitionBase!=NULL)
      CloseLibrary((struct Library *)IntuitionBase);

   if(GfxBase!=NULL)
      CloseLibrary((struct Library *)GfxBase);
}

/*Print the specified RastPort (whose ViewPort is pointed to by vp*/
BOOL dumpRastPort(struct RastPort *rp,struct ViewPort *vp)
{
#if 0
   struct IODRPReq *printerMsg;
   struct MsgPort *printerPort;
   static BOOL ableToPrint=TRUE;

   if(ableToPrint)
   {
      ableToPrint=FALSE;
      printerPort=CreatePort("2View.print.port",0);
      if(printerPort!=NULL)
      {
	 printerMsg=(struct IORequest *)CreateExtIO(printerPort,
				       (long)sizeof(struct IODRPReq));

	 if(printerMsg != NULL)
	 {
	    /*Open the printer device*/
	    if(OpenDevice("printer.device",0,printerMsg,0)==0)
	    {
	       /*Set up the IODRPReq structure*/
	       printerMsg->io_Command=PRD_DUMPRPORT;
	       printerMsg->io_RastPort=rp;
	       printerMsg->io_ColorMap=vp->ColorMap;
	       printerMsg->io_Modes=vp->Modes;
	       printerMsg->io_SrcX=0;
	       printerMsg->io_SrcY=0;
	       printerMsg->io_SrcWidth=vp->DWidth;
	       printerMsg->io_SrcHeight=vp->DHeight;
	       printerMsg->io_Special=SPECIAL_ASPECT|SPECIAL_FULLROWS|
				      SPECIAL_FULLCOLS;

	       /*Do it*/
	       if(DoIO(printerMsg)==0)
		  ableToPrint=TRUE;

	       CloseDevice(printerMsg);
	    }
	    DeleteExtIO(printerMsg);
	 }
	 DeletePort(printerPort);
      }
   }

   return(ableToPrint);
#else
    return 0;
#endif
}

/*Determine which colors to cycle, for a CRNG*/
UBYTE interpretCRNG(UBYTE *cycleTable,CRNG *crng,UBYTE *rate)
{
   UBYTE length=0;
   UBYTE pos,color;

      /*If the rate is zero, colors won't cycle anyway, so return 0*/
   if(crng->rate==0)
      return(0);

      /*Get the cycle rate*/
   *rate=16384/crng->rate;

      /*If the colors are actually suppossed to be cycling...*/
   if(crng->active!=0)
   {
	 /*Get the number of colors to cycle*/
      length=crng->high-crng->low+1;

	 /*If there are colors to cycle*/
      if(length!=0)
      {
	 if(crng->active==1)
	       /*Forward cycling*/
	    for(pos=0,color=crng->low;pos<length;pos++,color++)
	       cycleTable[pos]=color;
	 else
	       /*Backward cycling*/
	    for(pos=0,color=crng->high;pos<length;pos++,color--)
	       cycleTable[pos]=color;
      }
   }
   return(length);
}

UBYTE interpretDRNG(UBYTE *cycleTable,DRNG *drng,UBYTE *rate)
{
   UBYTE pos;
   DIndex *index;

      /*Skip past true-color values*/
   index=(DIndex *)((ULONG)drng+sizeof(DRNG)+4*drng->ntrue);

      /*Colors won't cycle if rate is zero, so return 0*/
   if(drng->rate==0)
      return(0);

   *rate=16384/drng->rate;

      /*If flags==0, there is no color cycling, so return*/
   if(drng->flags==0)
      return(0);

      /*Get the color registers to cycle*/
   for(pos=0;pos<drng->nregs;pos++)
   {
      cycleTable[pos]=index->index;
      index=(DIndex *)((ULONG)index+sizeof(DIndex));
   }

      /*Return the number of colors that are cycling*/
   return(drng->nregs);
}

/*Cycle a screen's colors according to the manner specified in cycleTable*/
void cycleColors(UBYTE *cycleTable,UWORD *colorTable,UBYTE length,
		 UBYTE numColors)
{
   UWORD tempColor;
   BYTE color;

      /*Get the first color in the cycle list*/
   tempColor=colorTable[cycleTable[length-1]];

      /*Shift each color in the list to its next place in the color table*/
   for(color=length-2;color>=0;color--)
      colorTable[cycleTable[color+1]]=colorTable[cycleTable[color]];

      /*The first color in the list became the last color */
   colorTable[cycleTable[0]]=tempColor;

   LoadRGB4(&(prevScreen->ViewPort),colorTable,numColors);
}

#if 0
/*Setup the copper list for dynamic hires images (as output by Macro Paint;*/
/*Digi-View dynamic hires images aren't supported yet).*/
void setupDynHires(struct Screen *scr,UWORD *colorBuf)
{
   UBYTE color;

      /*Get the first visible line on the screen*/
   UWORD line=(scr->TopEdge < 0) ? -(scr->TopEdge) : 0;

   struct UCopList *cl,*oldCl;

   LoadRGB4(&(scr->ViewPort),colorBuf,16);

   if(line > 10)
      line-=10;
   else
      line=0;

      /*Allocate the copper list header*/
   cl=(struct UCopList *)AllocMem(sizeof(struct UCopList),
				  MEMF_PUBLIC|MEMF_CHIP|MEMF_CLEAR);

      /*Return if there was no memory*/
   if(cl==NULL)
      return;

      /*Initialize the number of  copper list entries */
   CINIT(cl,17*2*scr->Height);

      /*If the image is interlaced, only get the colors for every other*/
      /*scan line (if we were to try to setup every scan line, the image*/
      /*wouldn't come out)*/
   if(scr->ViewPort.Modes & LACE)
      for(;line<scr->Height/2;line++)
      {
	 CWAIT(cl,(line-1)<<1,112);

	    /*Macro Paint only changes colors 4-16*/
	 for(color=4;color<16;color++)
	    CMOVE(cl,custom.color[color],colorBuf[(line<<5)+color]);
      }
   else
      for(;line<scr->Height;line++)
      {
	 CWAIT(cl,(line-1),112);

	 for(color=4;color<16;color++)
	    CMOVE(cl,custom.color[color],colorBuf[(line<<4)+color]);
      }

      /*Terminate the copper list*/
   CEND(cl);

      /*Install the new copper list, storing the previous one (if any)*/
   oldCl=scr->ViewPort.UCopIns;
   scr->ViewPort.UCopIns=cl;
   RethinkDisplay();

      /*If there was a previous copper list, free its memory*/
   if(oldCl!=NULL)
   {
      if(oldCl->FirstCopList != NULL)
	 FreeCopList(oldCl->FirstCopList);
      FreeMem(oldCl,sizeof(struct UCopList));
   }

   return;
}

/*Setup the copper list for a SHAM image*/
void setupSHAM(struct Screen *scr,UWORD *sham)
{
   int posInBuf=17;
   UBYTE color;

      /*Get the first visible line on the screen*/
   UWORD line=(scr->TopEdge < 0) ? -(scr->TopEdge) : 0;
   struct UCopList *cl,*oldCl;

      /*Start at a line before the first visible line */
   if(line > 10)
      line-=10;
   else
      line=0;

      /*Allocate the memory for the copper list header*/
   cl=(struct UCopList *)AllocMem(sizeof(struct UCopList),
				  MEMF_PUBLIC|MEMF_CHIP|MEMF_CLEAR);

   if(cl==NULL)
      return;

      /*Skip past the colors for the first line (which are the same */
      /*as the contents of the CMAP chunk*/
   posInBuf+=(16*line);

      /*Create the copper list*/
   for(;line<scr->Height;line++)
   {
      CWAIT(cl,line,200);
      for(color=0;color<16;color++)
	 CMOVE(cl,custom.color[color],sham[posInBuf++]);
   }

      /*Terminate it*/
   CEND(cl);

      /*Install it*/
   oldCl=scr->ViewPort.UCopIns;
   scr->ViewPort.UCopIns=cl;
   RethinkDisplay();

      /*Free the memory of the old copper list, if one existed*/
   if(oldCl!=NULL)
   {
      if(oldCl->FirstCopList != NULL)
	 FreeCopList(oldCl->FirstCopList);
      FreeMem(oldCl,sizeof(struct UCopList));
   }

   return;
}
#endif

/*End of Misc.c*/
