
/*ARexx.c:  This is the ARexx support code module*/
#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include "minrexx.h"
#include "arexx.h"

#include <proto/exec.h>

#define stcu_d(buf,val)     sprintf(buf,"%d",(int)val)
#define stci_h(buf,val)     sprintf(buf,"%x",(int)val)
#define stricmp 	    strcasecmp


enum ButtonTypes {none=0,select,menu};
typedef enum ButtonTypes ButtonTypes;

/* extern struct Library *SysBase;
extern struct Library *IntuitionBase; */
extern BYTE specialModes;

struct RexxMsg *rexxMsg;

/*These variables are defined in 2View.c, and are used to enable the ARexx*/
/*routines to communicate with the rest of the program*/

/*This variable is used to tell 2View to quit or advance*/
extern ButtonTypes rexxAbort;

extern struct Screen *prevScreen;   /*The current screen*/

extern char *picFilename;  /*The filename of the current picture*/
extern char *playListFilename;	 /*The name of the playlist*/

extern UWORD ticks;  /*The number of ticks remaining to show each picture*/
extern UWORD ticksRemaining;   /*The time left to show the current picture*/

extern BOOL loop;	   /*TRUE if the user specified 'loop'*/
extern BOOL printPics;	   /*TRUE if we're printing each picture*/

extern BOOL cycle;	   /*TRUE if colors are cycling*/
extern UBYTE rate,numCycleColors;

extern char *version;	   /*2View's version string*/

void rQuit(char *p);
void rAdvance(char *p);
void rGet(char *p);
void rPicToFront(char *p);
void rPicToBack(char *p);
void rPrint(char *p);
void rCycle(char *p);

struct rexxCommandList commandList[] =
{
   { "quit" , (APTR)&rQuit },     /*Abort a picture sequence*/
   { "advance", (APTR)&rAdvance },  /*Show the next picture in the sequence*/
   { "get", (APTR)&rGet },          /*Multi-purpose*/
   { "pictofront", (APTR)&rPicToFront }, /*Bring the picture to the front*/
   { "pictoback", (APTR)&rPicToBack },   /*Send the picture to the back*/
   { "print", (APTR)&rPrint },           /*Print the picture*/
   { "cycle", (APTR)&rCycle },           /*Turn color cycling on/off*/
   { NULL, NULL }
};

char portName[16];
char arexxBuf[140];


/*Open the ARexx port*/
long initRexxPort(void)
{
   determinePortName(portName);  /*Get the port name*/

   /*Ask minrexx to open the port and return the port's signal bit*/
   return(upRexxPort(portName,commandList,"rx",(APTR)&disp));
}

char *result="RESULT";
UBYTE errorNum=0;

/*Determine what the ARexx port name should be.  The first running instance*/
/*of 2View should have an ARexx port named '2View.1'.  The second, '2View.2'*/
/*etc.	This starts at '2View.1' and works up until it finds a free space*/
/*(up to 2View.99)*/
void determinePortName(char *portName)
{
   ULONG c=1;
   UBYTE len;

   strcpy(portName,"2View.");

   do
   {
      len=stcu_d(&portName[6],c++);
      portName[6+len]='\0';
   }
   while(FindPort(portName)!=NULL && c<100);
   if(FindPort(portName)!=NULL)
      exit(50);
   return;
}

/*The ARexx command dispatch function.	This calls the functions associated*/
/*with a particular command*/
int disp(struct RexxMsg *msg,struct rexxCommandList *dat,char *p)
{
   rexxMsg=msg;

   /*Call the function that supports the command*/
   ((int (*)())(dat->userdata))(p);

   /*Reply to ARexx, using the reply string and error number set by the*/
   /*function just called*/
   replyRexxCmd(rexxMsg,errorNum,0,arexxBuf);
   errorNum=0;
   return(1);  /*1 means that minrexx shouldn't reply to the rexx message*/
}

/*quit:  Abort a sequence of pictures*/
void rQuit(char *p)
{
   rexxAbort=menu;	/*It's like the user pressed the right mouse button*/
   strcpy(arexxBuf,result);
}

/*advance:  Advance to the next picture in this sequence*/
void rAdvance(char *p)
{
   rexxAbort=select;	/*Like pressing the left mouse button*/
   strcpy(arexxBuf,result);
}

/*get:	This function gets information about the current picture/playlist*/
void rGet(char *p)
{
   p++;

   /*The picture's filename*/
   if(stricmp(p,"name")==0)
      strcpy(arexxBuf,picFilename);

   /*The picture width*/
   else if(stricmp(p,"width")==0)
      stcu_d(arexxBuf,prevScreen->Width);

   /*The height*/
   else if(stricmp(p,"height")==0)
      stcu_d(arexxBuf,prevScreen->Height);

   /*The number of bitplanes*/
   else if(stricmp(p,"depth")==0)
      stcu_d(arexxBuf,prevScreen->BitMap.Depth);

   /*The viewmodes of the picture (HAM, HIRES, LACE, etc.*/
   else if(stricmp(p,"viewmodes")==0)
      stci_h(arexxBuf,prevScreen->ViewPort.Modes);

   else if(stricmp(p,"specialmodes")==0)
      switch(specialModes)
      {
	 case NORMAL:
	    strcpy(arexxBuf,"NONE");
	    break;
	 case SHAM:
	    strcpy(arexxBuf,"SHAM");
	    break;
	 case MACROPAINT:
	    strcpy(arexxBuf,"MACROPAINT");
	    break;
      }

   /*The time left to display this picture*/
   else if(stricmp(p,"timeleft")==0)
      stcu_d(arexxBuf,ticksRemaining);

   /*The time to display each picture*/
   else if(stricmp(p,"timeperpicture")==0)
      stcu_d(arexxBuf,ticks);

   /*The version number and date of this program*/
   else if(stricmp(p,"version")==0)
      strcpy(arexxBuf,&version[12]);

   /*The name of the playlist, or '?NONE?' if there is no playlist*/
   else if(stricmp(p,"playlistname")==0)
      if(playListFilename==NULL)
	 strcpy(arexxBuf,"?NONE?");
      else
	 strcpy(arexxBuf,playListFilename);

   /*Returns 'TRUE' if the user specified 'loop' on the command line, 'FALSE'*/
   /*if he didn't*/
   else if(stricmp(p,"looping")==0)
      if(loop)
	 strcpy(arexxBuf,"TRUE");
      else
	 strcpy(arexxBuf,"FALSE");

   /*Returns 'TRUE' if the user specified 'print' on the command line,*/
   /*FALSE if she didn't*/
   else if(stricmp(p,"printpictures")==0)
      if(printPics)
	 strcpy(arexxBuf,"TRUE");
      else
	 strcpy(arexxBuf,"FALSE");

   else if(stricmp(p,"cyclerate")==0)
      if(numCycleColors!=0)
	 stcu_d(arexxBuf,rate);
      else
	 strcpy(arexxBuf,"-VOID-");

   else if(stricmp(p,"cyclestatus")==0)
      if(cycle)
	 strcpy(arexxBuf,"CYCLING");
      else
	 if(numCycleColors!=0)
	    strcpy(arexxBuf,"CYCLINGPAUSED");
	 else
	    strcpy(arexxBuf,"NOCYCLING");

   /*Otherwise, we don't understand, so return an error*/
   else
   {
      errorNum=100;
      strcpy(arexxBuf,"ERROR");
   }
}

/*pictofront:  Send the picture's screen to the front*/
void rPicToFront(char *p)
{
   ScreenToFront(prevScreen);
   strcpy(arexxBuf,result);
}

/*pictoback:  Send the picture's screen to the back*/
void rPicToBack(char *p)
{
   ScreenToBack(prevScreen);
   strcpy(arexxBuf,result);
}

/*print:  Print the currently displayed picture*/
void rPrint(char *p)
{
   dumpRastPort(&(prevScreen->RastPort),&(prevScreen->ViewPort));
   strcpy(arexxBuf,result);
}

/*cycle:  Turn color cycling on and off*/
void rCycle(char *p)
{
   p++;

   if(stricmp(p,"on")==0)
   {
      if(!cycle)
	 toggleCycling();
   }
   else if(stricmp(p,"off")==0)
   {
      if(cycle)
	 toggleCycling();
   }
   else if(stricmp(p,"toggle")==0)
      toggleCycling();

   strcpy(arexxBuf,result);
   return;
}

/*End of ARexx.c*/

