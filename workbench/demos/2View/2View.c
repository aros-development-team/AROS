/* Modified version for AROS - The Amiga Research OS
** $Id$
*/

/***********************************************************************\
*                            2View V1.50                                *
*        A simple, fast ILBM viewer, for use under AmigaOS V2.x.        *
*    Written and ©1991-1992 by Dave Schreiber.  All Rights Reserved.    *
*                                                                       *
* Usage:                                                                *
*  2View FILE/A/M,FROM/K,SECS=SECONDS/K/N,TICKS/K/N,LOOP/S,PRINT        *
*                                                                       *
*  Where the following arguments are defined as follows:                *
*   FILE - The name of one (or more) IFF ILBM files                     *
*   FROM - A file containing a list of filenames.  Used instead of FILE *
*   SECS - Number of seconds to display a file                          *
*   TICKS - Number of ticks (1/60ths of a second)                       *
*   LOOP - When finished showing the last pictures, start over          *
*   PRINT - Print each picture as it is shown                           *
*                                                                       *
*  To compile (with SAS/C V5.10a):                                      *
*     lc -v 2View ARexx                                                 *
*     lc -v -b0 Misc                                                    *
*     asm 2ViewAsm.a                                                    *
*     blink with 2View.lnk                                              *
*                                                                       *
*  Version history:                                                     *
*     1.50 - Rewrote the subroutine that reads the ILBM from disk in    *
*            assembly language, for speed.  Added support for SHAM and  *
*            Macro Paint images, and for color cycling (both            *
*            traditional (CRNG, i.e. continutout cycle ranges) and      *
*            DPaint-IV style cycling (DRNG, i.e. non-continuous cycle   *
*            ranges).  A 'tick' (as used with the "TICK" keyword, above)*
*            has been redefined as 1/60th of a second.  Finally, the    *
*            source code in 2View.c has been split into two files       *
*            (2View.c and  Misc.c).                                     *
*            Released 3/24/92                                           *
*                                                                       *
*     1.11 - Improved error reporting (with this version, if the user   *
*            run 2View from Workbench and there's an error, a requester *
*            is put up.  Previously, the user was not notified at all   *
*            of the error).                                             *
*            Released 9/11/91                                           *
*                                                                       *
*     1.10 - Added support for Workbench, ARexx, scrollable bitmaps,    *
*            and printing.  Also, the user can now use CTRL-C to advance*
*            to the next frame, and CTRL-D to abort a playlist.         *
*            Released 9/3/91                                            *
*                                                                       *
*     1.00 - Original version.  Released 7/24/91                        *
*                                                                       *
\************************************************************************/


unsigned long availBytes,curPos,bufSize;

/*Include files*/

#include <exec/types.h>
#include <libraries/iffparse.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <exec/memory.h>
#include <workbench/startup.h>
#include <graphics/gfxbase.h>

/*Prototypes*/
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>

/*Other include files*/
#include "iff.h"
#include "2View.h"
/* #include "arexx.h" */

#include <stdlib.h>
#include <string.h>

/*Libraries we'll need*/
struct Library *IFFParseBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;

#ifdef __AROS__
#define __chip
#endif

/*Generic screen and window definitions.  They will be used to define*/
/*the screen and window that the various pictures will be shown on*/
struct NewScreen newScreen=
{
   0,0,0,0,0,1,0,0,CUSTOMSCREEN|SCREENBEHIND|AUTOSCROLL,NULL,NULL,NULL,
   NULL
};

struct NewWindow newWindow =
{
   0,0,0,0,0,1,IDCMP_MENUPICK|IDCMP_MOUSEBUTTONS|IDCMP_ACTIVEWINDOW|IDCMP_VANILLAKEY,
      WFLG_RMBTRAP|WFLG_BORDERLESS|WFLG_NOCAREREFRESH,NULL,NULL,NULL,NULL,NULL,
      0,0,640,400,CUSTOMSCREEN
};

struct Screen *screen=NULL;
struct Window *window=NULL;

UWORD *storage;
int counter;

/*A true here indicates the current ILBM file is compressed*/
BYTE Compression;

/*The version string.  Do a 'version 2View' to display it*/
char *version="$VER: QView V1.50 (24.3.92)";

/*Just so that the © message is part of the actual program*/
char *copyRightMsg="Copyright 1991-1992 by Dave Schreiber.  All Rights Reserved.";

BYTE ExitFlag=FALSE;    /*'Exit now' flag*/
UWORD ticks=0;        /*Delay requested by user.*/

/*The previous screen and window*/
struct Window *prevWindow=NULL;
struct Screen *prevScreen=NULL;

/*Data for a blank pointer*/
UWORD __chip fakePointerData[]={0,0,0,0,0};

struct IFFHandle *iff=NULL;   /*IFF handle*/
BPTR pL=NULL;                 /*Playlist file pointer*/
BOOL masking,print,toFront,printPics;
#ifdef __AROS__
struct WBStartup *WBenchMsg = NULL;
#else
extern struct WBStartup *WBenchMsg;
#endif

char *playListFilename=NULL;

/*Variables that have to be global so that ARexx.c can access them*/
ButtonTypes rexxAbort=none;
/* long arexxSigBit; */
UWORD ticksRemaining=0;
BOOL loop=FALSE;
BYTE specialModes;

char *picFilename;
char trashBuf[512];       /* A place to dump mask information */
char *buf=trashBuf;

struct TagItem TagList[]=
{
      /* This defines what part of the displayed picture is shown.  It's */
      /* necessary to have a line like this in here in order to get      */
      /* 2.0 autoscrolling to work.                                      */
   {SA_Overscan,OSCAN_VIDEO},
   {TAG_DONE,0}
};

char *about1="2View";
char *about2="Please";

extern struct EasyStruct erError1Line;
BOOL cycle=FALSE;
UBYTE numColors;
UWORD destMap[32];
UBYTE numCycleColors;
UBYTE rate;

/* The assembly-language reader routine */
extern int ReadILBM (struct IFFHandle * iff,
    struct Window * window, ULONG width, ULONG height, UWORD Depth,
        BOOL Compression, BOOL masking);

char **filenames;
UWORD numFilenames=0,numSlots;

int main(int argc, char ** argv)
{
   UWORD c;
   IPTR args[7] = { 0 };
   char **filenames = NULL;
   char curFilename[140];
   BYTE playList = FALSE; /*True if a playlist is being used, false otherwise*/

      /*Open the libraries*/
   IFFParseBase=(struct Library *)OpenLibrary("iffparse.library",0L);
   if(IFFParseBase==NULL)
   {
      cleanup();
      exit(50);
   }

   IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0L);
   if(IntuitionBase==NULL)
   {
      cleanup();
      exit(75);
   }

   GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",0L);
   if(GfxBase==NULL)
   {
      cleanup();
      exit(85);
   }

      /*Get the arguments*/
   if(WBenchMsg==NULL)
   {
      ParseArgs(args);

         /*If a playlist filename was provided, store it for later use*/
      if((char *)args[4]!=NULL)
      {
         playListFilename=(char *)args[4];
         playList=TRUE;
      }
      else  /*Otherwise, read the filenames from the command line*/
         playList=FALSE;

         /*If a time was provided (in ticks), use it*/
      if((ULONG *)args[1]!=NULL)
         ticks=*(ULONG *)args[1]*50;

         /*If a time was provided (in seconds), use it (overrides ticks)*/
      if((ULONG *)args[2]!=NULL && *(ULONG *)args[2]!=0)
         ticks=*(ULONG *)args[2];

         /*If neither a picture filename, nor a playlist filename, was*/
         /*specified, print an error and exit.*/
      if((char **)args[0]==NULL && !playList)
      {
         printError("Please enter one or more filenames");
         cleanup();
         exit(45);
      }

         /*Determine if we should print the pictures we display or not*/
      printPics=((BOOL *)args[5]!=NULL);

         /*Get the pointer to the list of filename*/
      filenames=(char **)args[0];

         /*Will we loop back to the beginning once we finish displaying all*/
         /*the pictures?*/
      loop=((BOOL *)args[3]!=NULL);
   }
   else
      if(WBenchMsg->sm_NumArgs==1)
      {
         EasyRequest(NULL,&erError1Line,NULL,
                     (ULONG) "2View V1.50 (March 24, 1992)",
                     (ULONG) "Written by Dave Schreiber");
         cleanup();
         exit(0);
      }


#if 0
      /* Initialize the ARexx port */
   arexxSigBit=initRexxPort();
   if(arexxSigBit==0)
   {
      cleanup();
      exit(47);
   }
#endif

      /*Allocate the IFF structure*/
   iff=AllocIFF();

      /*If the allocation failed, abort*/
   if(iff==NULL)
   {
      printError("Couldn't allocate necessary resources");
      cleanup();
      exit(100);
   }

      /*Run until we run out of filenames, or the user aborts*/
   while(!ExitFlag)
   {
      picFilename=curFilename;   /*Get a pointer to the filename buffer*/

         /*Check to see if we're running from Workbench.  If so, and the*/
         /*user provided names of pictures to display (by clicking on their*/
         /*icons), display those pictures*/
      if(WBenchMsg!=NULL && WBenchMsg->sm_NumArgs>1)
      {
         CurrentDir(WBenchMsg->sm_ArgList[1].wa_Lock);
         picFilename=WBenchMsg->sm_ArgList[1].wa_Name;
      }
      else if(playList) /*If a playlist is being used*/
      {
         pL=Open(playListFilename,MODE_OLDFILE);   /*Open the playlist*/

         if(pL==NULL)   /*If we couldn't open the playlist, abort*/
         {
            printError("Can't open playlist");
            cleanup();
            exit(199);
         }

         do    /*Loop until we run out of playlist, or get a valid name*/
         {
            if(FGets(pL,picFilename,140)==NULL) /*If end-of-file*/
               picFilename=NULL;       /*Set as NULL as a flag*/
         }
         while(picFilename!=NULL && picFilename[0]==0x0A);

         if(picFilename!=NULL)        /*If not NULL, it's a valid filename*/
            picFilename[strlen(picFilename)-1]='\0'; /*Remove the linefeed*/
      }
      else  /*Otherwise, if a playlist isn't being used, get the current*/
         picFilename=filenames[0];     /*filename*/


         /*Loop while the user hasn't requested an abort, and while*/
         /*there are still files to display*/
      for(c=0;!ExitFlag && picFilename!=NULL;c++)
      {
         if((iff->iff_Stream=(IPTR)Open(picFilename,MODE_OLDFILE))==0)
         {     /*If the ILBM file can't be opened...*/

               /*Print an error...*/
            printError("Can't open: %s", picFilename);

            cleanup();
            exit(200);
         }

         InitIFFasDOS(iff);      /*The IFF file will be read from disk*/

         OpenIFF(iff,IFFF_READ); /*Make iffparse.library aware of the*/
                                 /*ILBM file*/

         /*Read in the file and display*/
         ReadAndDisplay(picFilename,iff);

         CloseIFF(iff);          /*Release iffparse's hold on the file*/

         Close((BPTR)iff->iff_Stream); /*Close the file*/

            /*Get the next filename, either from Workbench,*/
         if(WBenchMsg!=NULL)
         {
            if(WBenchMsg->sm_NumArgs > c+2)
            {
               CurrentDir(WBenchMsg->sm_ArgList[c+2].wa_Lock);
               picFilename=WBenchMsg->sm_ArgList[c+2].wa_Name;
            }
            else
               picFilename=NULL;
         }
         else if(playList)   /*The playlist*/
         {
            do
            {
               if(FGets(pL,picFilename,140)==NULL)
                  picFilename=NULL;
            }
            while(picFilename!=NULL && picFilename[0]==0x0A);

            if(picFilename!=NULL)
               picFilename[strlen(picFilename)-1]='\0';
         }
         else  /*or the command line*/
            picFilename=filenames[c+1];
      }

         /*We're finished with this run of pictures*/
      if(playList)         /*Close playlist, if open*/
         Close(pL);
      pL=NULL;

      if(!loop && !printPics) /*If the loop flag wasn't given, exit*/
         ExitFlag=TRUE;
   }

      /*Time to exit, so close stuff*/

   cleanup();
   exit(0); /*And exit*/
}

LONG ilbmprops[] = { ID_ILBM,ID_CMAP,ID_ILBM,ID_BMHD,ID_ILBM,ID_CAMG,
                     ID_ILBM,ID_CRNG,ID_ILBM,ID_DRNG,ID_ILBM,ID_SHAM,
                     ID_ILBM,ID_CTBL };

/*Read in an ILBM file and display it*/
void ReadAndDisplay(char *filename,struct IFFHandle *iff)
{
   int error;
   UBYTE *bodyBuffer = NULL;   /*Pointer to buffer holding 'BODY' chunk info*/
   ULONG ViewModes;     /*Holds the viewmodes flags*/
   UWORD c;
   ButtonTypes button;
   UBYTE cycleTable[32];
   UBYTE countDown;

      /*Structures required for IFF parsing*/
   struct StoredProperty *bmhd,*cmap,*camg,*crng,*drng,*sham,*ctbl;
   struct ContextNode *bodyContext;

      /*IntuiMessage...*/
   struct IntuiMessage *mesg;

      /*Indentify chunks that should be stored during parse*/
      /*(in this case, CMAP, BMHD, CRNG, DRNG, CAMG, and SHAM)*/
   error=PropChunks(iff,ilbmprops,7);

      /*If there was an error, print a message and return*/
   if(error!=0)
   {
      printError("Error in PropChunks() wile reading %s: %d\n",filename,error);
      ExitFlag=TRUE;
      return;
   }

      /*Tell iffparse to stop at a 'BODY' chunk*/
   error=StopChunk(iff,ID_ILBM,ID_BODY);

      /*Error handling yet again*/
   if(error!=0 && error!=-1)
   {
      printError("Error in StopChunk() wile reading %s: %d\n",filename,error);
      ExitFlag=TRUE;
      return;
   }

      /*Do the actual parsing*/
   error=ParseIFF(iff,IFFPARSE_SCAN);

      /*Check for errors yet again*/
   if(error!=0 && error !=-1)
   {
      printError("Error in ParseIFF() wile reading %s: %d\n",filename,error);
      ExitFlag=TRUE;
      return;
   }

      /*Get the chunks that were found in the file*/
   bmhd = FindProp(iff,ID_ILBM,ID_BMHD);  /*Bitmap information*/
   cmap = FindProp(iff,ID_ILBM,ID_CMAP);  /*Color map*/
   camg = FindProp(iff,ID_ILBM,ID_CAMG);  /*Amiga viewmode information*/
   crng = FindProp(iff,ID_ILBM,ID_CRNG);  /*Color-cycling ranges*/
   drng = FindProp(iff,ID_ILBM,ID_DRNG);  /*New (DPaint IV) color-cycling*/
   sham = FindProp(iff,ID_ILBM,ID_SHAM);  /*SHAM color tables*/
   ctbl = FindProp(iff,ID_ILBM,ID_CTBL);  /*Macro Paint color table info*/

      /*Get the descriptor for the BODY chunk*/
   bodyContext=CurrentChunk(iff);

      /*If there wasn't a BMHD, CMAP, or BODY chunk, abort*/
   if (!bmhd | !cmap | !bodyContext)
   {
      printError ("%s is corrupted or is not in Amiga ILBM format. No %s%s%s%s%s found."
        , filename
        , bmhd ? "" : "BMHD"
        , !bmhd && !cmap ? ", " : ""
        , cmap ? "" : "CMAP"
        , (!bmhd || !cmap) && !bodyContext ? ", " : ""
        , bodyContext ? "" : "BODY"
      );
      ExitFlag=TRUE;
      return;
   }

      /*Prepare to determine screen modes*/
   newScreen.ViewModes=0;

      /*If there was a CAMG chunk, use it to get the viewmodes*/
   if(camg!=NULL)
   {
      ViewModes=( ((CAMG *)(camg->sp_Data))->viewmodes );

      if(ViewModes & HAM)
         newScreen.ViewModes|=HAM;

      if(ViewModes & EXTRA_HALFBRITE)
         newScreen.ViewModes|=EXTRA_HALFBRITE;

      if(ViewModes & LACE)
         newScreen.ViewModes|=LACE;

      if(ViewModes & HIRES)
         newScreen.ViewModes|=HIRES;
   }


   if(crng==NULL)
   {
      if(drng==NULL) /*No color cycling*/
         numCycleColors=0;
      else  /* DPaint-IV--style color cycling*/
         numCycleColors=interpretDRNG(cycleTable,(DRNG *)(drng->sp_Data),&rate);
   } else  /*DPaint I-III--style color cycling*/
      numCycleColors=interpretCRNG( cycleTable,(CRNG *)(crng->sp_Data),&rate);

   if(numCycleColors != 0)
      cycle=TRUE;    /*Start cycling if there are colors to cycle*/
   else
      cycle=FALSE;

      /*Interpret the BMHD chunk*/
   getBMHD(bmhd->sp_Data);

      /*Don't open an interlace screen if the image is in SHAM mode*/
      /*(the Amiga OS doesn't properly handle user copper lists on */
      /*interlaced screens for some reason)*/
   if(sham!=NULL)
      newScreen.ViewModes&=~LACE;

      /*Open a screen, defined by the BMHD and CAMG chunks*/
   screen=OpenScreenTagList(&newScreen,TagList);

      /*If the screen couldn't be opened, abort*/
   if(screen==NULL)
   {
      printError("Cannot open screen!");
      ExitFlag=TRUE;
      return;
   }

      /*This more properly centers the screen, for some reason */
   MoveScreen(screen,1,1);
   MoveScreen(screen,-1,-1);

      /*Set the window dimensions from the screen dimensions*/
   newWindow.Screen=screen;
   newWindow.Width=newScreen.Width;
   newWindow.Height=newScreen.Height;

      /*Open the window*/
   window=OpenWindow(&newWindow);

      /*Abort if the window couldn't be opened*/
   if(window==NULL)
   {
      printError("Cannot open window!");

      ExitFlag=TRUE;
      return;
   }

   availBytes = bufSize;
   curPos = bufSize;

      /*Blank out the pointer*/
   SetPointer(window,fakePointerData,1,16,0,0);

      /*Set the screen colors to those provided in the CMAP chunk*/
   setScreenColors(screen,cmap->sp_Data,newScreen.Depth,destMap,&numColors);

      /*Uncompress an ILBM and copy it into the bitmap*/
   if (!ReadILBM (iff, window, window->Width, window->Height,
        newScreen.Depth,
        Compression, masking))
   {
      printError ("Cannot read bitmap!");

      ExitFlag=TRUE;
      return;
   }

      /*Activate the window, and flush any IDCMP message*/
   ActivateWindow(window);
   while((mesg=(struct IntuiMessage *)GetMsg(window->UserPort))!=NULL)
      ReplyMsg((struct Message *)mesg);

#if 0
      /*If this is a SHAM image, setup the copper list appropriately*/
   if(sham!=NULL)
   {
      specialModes=SHAM;
      setupSHAM(screen,(UWORD *)(sham->sp_Data));
   }
   else
         /*If this is a MacroPaint image, setup the copper list*/
      if(ctbl!=NULL)
      {
         specialModes=MACROPAINT;
         setupDynHires(screen,(UWORD *)(ctbl->sp_Data));
      }
      else
#endif
            /* Otherwise, this is a normal ILBM*/
         specialModes=NORMAL_MODE;

      /*Bring the screen to the front*/
   ScreenToFront(screen);

      /*If the user used the 'print' flag on the command line, print*/
      /*the picture (but not if this is a SHAM or MacroPaint image)*/
   if(printPics && specialModes == NORMAL_MODE)
      dumpRastPort(&(screen->RastPort),&(screen->ViewPort));

   print=TRUE;

      /*Close the previous window and screen*/
   if(prevWindow!=NULL)
      CloseWindow(prevWindow);
   if(prevScreen!=NULL)
      CloseScreen(prevScreen);

      /*Free the buffer that holds the BODY chunk information*/
   FreeMem(bodyBuffer,bufSize);

      /*Store the current window & screen structures, so they can be*/
      /*closed later*/
   prevWindow=window;
   prevScreen=screen;

   screen=NULL;
   window=NULL;

   rexxAbort=none;
   countDown=rate;
   if(ticks==0)   /*If ticks==0, this means that no delay was specified*/
   {              /*by the user.  So just wait for him to click a button*/
#if 0
      int prevTopEdge=prevScreen->TopEdge;
#endif

      while((button=checkButton())==none && rexxAbort==none)
      {
            /*Wait for 1/60th of a second*/
         Wait (1L << prevWindow->UserPort->mp_SigBit);

#if 0
            /*Refresh the SHAM copper list if required*/
         if(prevTopEdge!=prevScreen->TopEdge && sham!=NULL)
         {
            prevTopEdge=prevScreen->TopEdge;
            setupSHAM(prevScreen,(UWORD *)(sham->sp_Data));
         }

            /*Refresh the MacroPaint copper list if required*/
         if(prevTopEdge!=prevScreen->TopEdge && ctbl!=NULL)
         {
            prevTopEdge=prevScreen->TopEdge;
            setupDynHires(prevScreen,(UWORD *)(ctbl->sp_Data));
         }
#endif

            /*If its time to cycle the colors, then cycle them*/
         if(cycle && numCycleColors!=0 && --countDown==0)
         {
            cycleColors(cycleTable,destMap,numCycleColors,numColors);
            countDown=rate;
         }

#if 0
         dispRexxPort();
#endif
      }

         /*Check to see if the user wants to abort*/
      if(button==menu || rexxAbort==menu)
         ExitFlag=TRUE;
   }
   else     /*Otherwise, wait for the specified amount of time*/
   {
      for(c=0;c<ticks;c++)
      {
            /*Wait 1/60th of a second*/
         WaitTOF();

            /*Cycle colors if necessary*/
         if(cycle && numCycleColors!=0 && --countDown==0)
         {
            cycleColors(cycleTable,destMap,numCycleColors,numColors);
            countDown=rate;
         }

#if 0
         dispRexxPort();         /*Check ARexx port*/
#endif

         button=checkButton();   /*After each 25 ticks, check to see if*/
         if(button==menu || rexxAbort==menu)    /*the user wants to abort*/
         {
            ExitFlag=TRUE;
            return;
         }
         if(button==select || rexxAbort==select)  /*Or advance prematurely*/
            return;
      }
   }
}

/*End of 2View.c*/

