/*************************************************************/
/* Includes and other common stuff for the Audex Project     */
/*************************************************************/

/* MUI */
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <libraries/mui.h>

/* System */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/exall.h>
#include <datatypes/soundclass.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <datatypes/pictureclass.h>
#include <libraries/usbclass.h>

/* ANSI C */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern struct ExecBase *SysBase;

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#define LabelB(label)   (void *)MUI_NewObject(MUIC_Text,MUIA_Text_PreParse,"\33r",\
            MUIA_Text_Contents,label,TextFrame,MUIA_FramePhantomHoriz,\
            TRUE,MUIA_Weight,0,MUIA_InnerLeft,0,MUIA_InnerRight,0,TAG_DONE)

#ifndef min
#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif

/* prototypes */

BOOL WriteConfig(STRPTR name);
void LoadPsdStackloader(void);
BOOL GetToolTypes(void);

void CleanupClasses(void);
BOOL SetupClasses(void);
void fail(char *str);
int main(int argc, char *argv[]);

/* extern variables */

extern struct MUI_CustomClass *ActionClass;
extern struct MUI_CustomClass *IconListClass;
extern struct MUI_CustomClass *DevWinClass;
extern struct MUI_CustomClass *CfgListClass;
extern BOOL registermode;

extern struct Library *MUIMasterBase;
extern struct Library *PsdBase;
extern struct WBStartup *_WBenchMsg;

