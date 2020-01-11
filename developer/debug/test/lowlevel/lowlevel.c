/****************************************************************/
/* lowlevel.c                                                   */
/****************************************************************/
/*                                                              */
/* Tiny program to check what lowlevel.library can do           */
/*                                                              */
/* Modification history                                         */
/* ====================                                         */
/* 13-Apr-2012 shows buttons                                    */
/* 04-Mar-2012 lowlevel_ext.h Rumble                            */
/* 15-Nov-2007 GP                                               */
/****************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <libraries/lowlevel.h>
#include <libraries/lowlevel_ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Library *LowLevelBase ;

void openlibs(void)
{
  LowLevelBase  = OpenLibrary("lowlevel.library", 0) ;
  IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0) ;
  GfxBase       = (struct GfxBase *)OpenLibrary("graphics.library", 0) ;
}

void closelibs(void)
{
  if (LowLevelBase != NULL)  CloseLibrary(LowLevelBase) ;
  if (IntuitionBase != NULL) CloseLibrary((struct Library *)IntuitionBase) ;
  if (GfxBase != NULL)       CloseLibrary((struct Library *)GfxBase) ;
}

char *joytype(ULONG joy)
{
  char *jtype ;

  switch (joy & JP_TYPE_MASK)
  {
    case JP_TYPE_NOTAVAIL : jtype = "not available "; break ;
    case JP_TYPE_GAMECTLR : jtype = "game controler"; break ;
    case JP_TYPE_MOUSE    : jtype = "mouse         "; break ; 
    case JP_TYPE_JOYSTK   : jtype = "joystick      "; break ;
    case JP_TYPE_UNKNOWN  : jtype = "unknown       "; break ;
    case JP_TYPE_ANALOGUE : jtype = "analogue      "; break ;
    default :               jtype = "?             "; break ;
  }

  return jtype ;
}

void DrawRect(struct RastPort *rp, int left, int top, int right, int bottom)
{
  Move(rp, left, top) ;
  Draw(rp, right, top) ;
  Draw(rp, right, bottom) ;
  Draw(rp, left, bottom) ;
  Draw(rp, left, top) ;
}

void Plot(struct RastPort *rp, int left, int top)
{
  RectFill(rp, left-2, top-2, left+2, top+2) ;
}

int forces[4] = {0, 0, 0, 0} ;
int forcef[4] = {0, 0, 0, 0} ;

void joyaction(struct RastPort *rp, int left, int top, int height, int i, ULONG joy)
{
  int fs = 0x00 ;
  int ff = 0x00 ;
  int x, y ;
  int px, py ;

  struct TagItem tags[3] ;

  switch (joy & JP_TYPE_MASK)
  {
    case JP_TYPE_GAMECTLR :
    case JP_TYPE_MOUSE    :
    case JP_TYPE_JOYSTK   :
    case JP_TYPE_ANALOGUE :
    {
      if (joy & JPF_BUTTON_RED)
      { 
        switch (i)
        {
          case 0 :
          case 1 :
          case 2 :
          case 3 :
          {
            forces[i] += 7 ;
            if (forces[i] > 255) forces[i] = 255 ;
            fs = forces[i] ;
            break ;
          }
        }
      }

      if (joy & JPF_BUTTON_BLUE)
      {
        switch (i)
        {
          case 0 :
          case 1 :
          case 2 :
          case 3 :
          {
            forcef[i] += 7 ;
            if (forcef[i] > 255) forcef[i] = 255 ;
            ff = forcef[i] ;
            break ;
          }
        }
      }

      if ((joy & JPF_BUTTON_RED) || 
          (joy & JPF_BUTTON_BLUE))
      {
        tags[0].ti_Tag  = SJA_RumbleSetSlowMotor ;
        tags[0].ti_Data = fs ;
        tags[1].ti_Tag  = SJA_RumbleSetFastMotor ;
        tags[1].ti_Data = ff ;
        tags[2].ti_Tag  = TAG_DONE ;
        tags[2].ti_Data = 0 ;
        
        SetJoyPortAttrsA(i, &tags[0]) ;
      }
      else
      {
        switch (i)
        {
          case 0 :
          case 1 :
          case 2 :
          case 3 :
          {
            if ((forces[i] != 0) || (forcef[i] != 0))
            {
              forces[i] = 0 ;
              forcef[i] = 0 ;

              tags[0].ti_Tag  = SJA_RumbleOff ;
              tags[0].ti_Data = 0 ;
              tags[1].ti_Tag  = TAG_DONE ;
              tags[1].ti_Data = 0 ;
        
              SetJoyPortAttrsA(i, &tags[0]) ;
            }
            break ;
          }
        }
      }
      break ;
    }

    default :
    {
      break ;
    }
  }

  /* Erase background */
  SetAPen(rp, 0) ;
  RectFill(rp, left+10-2, top+1-2, left+10+height+2, top+1+height+2) ;
 
  switch (joy & JP_TYPE_MASK)
  {
    case JP_TYPE_MOUSE    :
    case JP_TYPE_ANALOGUE :
    {
      SetAPen(rp, 1) ;
      DrawRect(rp, left+10, top+1, left+10+height, top+1+height) ;

      x = (LONG)(WORD)(BYTE)((joy >> 0) & 0xff) ; /* -128 .. 127 */
      y = (LONG)(WORD)(BYTE)((joy >> 8) & 0xff) ; /* -128 .. 127 */
      x += 128 ; /* 0 .. 255 */
      y += 128 ; /* 0 .. 255 */ 

      px = left+10+2+x*(height-4)/0xff ;
      py = top+1+2+y*(height-4)/0xff ;

      SetAPen(rp, 3) ;
      Plot(rp, px, py) ;

      if (joy & JPF_BUTTON_RED)
      {
        SetAPen(rp, 2) ;
        DrawRect(rp, px-2, py-2, px+2, py+2) ;
      }

      if (joy & JPF_BUTTON_BLUE)
      {
        SetAPen(rp, 2) ;
        DrawRect(rp, px-1, py-1, px+1, py+1) ;
      }
      break ;
    }

    case JP_TYPE_GAMECTLR :
    case JP_TYPE_JOYSTK   :
    {
      SetAPen(rp, 1) ;
      Move(rp, left+10+height/2, top) ;
      Draw(rp, left+10+height/2, top+height) ;
      Move(rp, left+10, top+height/2) ;
      Draw(rp, left+10+height, top+height/2) ;

      SetAPen(rp, 3) ;
      if (joy & JPF_JOY_UP)
      {
        if (joy & JPF_JOY_LEFT)
        {
          /* UP LEFT */
          px = left+12 ;
          py = top+2 ;
        }
        else if (joy & JPF_JOY_RIGHT)
        {
          /* UP RIGHT */
          px = left+10+height-2 ;
          py = top+2 ;
        }
        else
        {
          /* UP */
          px = left+10+height/2 ;
          py = top+2 ;
        }
      }
      else if (joy & JPF_JOY_DOWN)
      {
        if (joy & JPF_JOY_LEFT)
        {
          /* DOWN LEFT */
          px = left+12 ;
          py = top+height-2 ;
        }
        else if (joy & JPF_JOY_RIGHT)
        {
          /* DOWN RIGHT */
          px = left+10+height-2 ;
          py = top+height-2 ;
        }
        else
        {
          /* DOWN */
          px = left+10+height/2 ;
          py = top+height-2 ;
        }
      }
      else
      {
        if (joy & JPF_JOY_LEFT)
        {
          /* LEFT */
          px = left+10 ;
          py = top+height/2 ;
        }
        else if (joy & JPF_JOY_RIGHT)
        {
          /* RIGHT */
          px = left+10+height-2 ;
          py = top+height/2 ;
        }
        else
        {
          /* CENTRAL */
          px = left+10+height/2 ;
          py = top+height/2 ;
        }
      }
      Plot(rp, px, py) ;

      if (joy & JPF_BUTTON_RED)
      {
        SetAPen(rp, 2) ;
        DrawRect(rp, px-2, py-2, px+2, py+2) ;
      }

      if (joy & JPF_BUTTON_BLUE)
      {
        SetAPen(rp, 2) ;
        DrawRect(rp, px-1, py-1, px+1, py+1) ;
      }
      break ;
    }
  }
}

struct NewWindow nw ;
char msg[256] ;

#if !defined(__AROS__)
void __stdargs __main(char *line)
{
  exit(main(0, NULL)) ;
}
#endif

int main (int argc, char *argv[])
{
  struct Window *w = NULL ;
  struct IntuiMessage *imsg = NULL ;
  int loop ;
  int left, top ;
  int fh ;
  int i ;
  ULONG key, joy ;
  UBYTE *action = NULL ;

  openlibs() ;

  memset(&nw, 0, sizeof(nw)) ;
  nw.LeftEdge    = 10 ;
  nw.TopEdge     = 20 ;
  nw.Width       = 440 ;
  nw.Height      = 160 ;
  nw.DetailPen   = 0 ;
  nw.BlockPen    = 1 ;
  nw.IDCMPFlags  = IDCMP_CLOSEWINDOW   ;
  nw.Flags       = WFLG_DRAGBAR        |
                   WFLG_DEPTHGADGET    |
                   WFLG_CLOSEGADGET    |
                   WFLG_GIMMEZEROZERO  |
                   WFLG_SIMPLE_REFRESH ;
  nw.FirstGadget = NULL ; /* Pointer to first gadget */
  nw.CheckMark   = NULL ; /* Pointer to checkmark */
  nw.Title       = "lowlevel.library test",
  nw.Screen      = NULL ; /* screen pointer */
  nw.BitMap      = NULL ; /* bitmap pointer */
  nw.MinWidth    = 200 ;
  nw.MinHeight   = 80 ;
  if (((struct Library *)IntuitionBase)->lib_Version >= 33)
  {
    nw.MaxWidth    = 0xffff ;
    nw.MaxHeight   = 0xffff ;
  }
  else
  {
    nw.MaxWidth    = 640 ;
    nw.MaxHeight   = 256 ;
  }
  nw.Type        = WBENCHSCREEN	; /* type of screen */

  w = OpenWindow(&nw) ;
  if (w != NULL)
  {
    if (LowLevelBase != NULL)
    {
      loop = TRUE ;
      do
      {
        SetBPen(w->RPort, 0) ;
        SetAPen(w->RPort, 1) ;

        left = 10 ;
        top  = 10+w->RPort->TxBaseline ;
        fh   = w->RPort->TxHeight ;

        sprintf(msg, "lowlevel.library %ld.%ld",
                LowLevelBase->lib_Version, 
                LowLevelBase->lib_Revision ) ;
        Move(w->RPort, left, top+fh-w->RPort->TxBaseline) ;
        Text(w->RPort, msg, strlen(msg)) ;
        top += fh ;

        sprintf(msg, "GetLanguageSelection %ld", GetLanguageSelection()) ;
        Move(w->RPort, left, top+fh-w->RPort->TxBaseline) ;
        Text(w->RPort, msg, strlen(msg)) ;
        top += fh ;

        key = GetKey() ;
        sprintf(msg, "GetKey             0x%08lx", key) ;
        Move(w->RPort, left, top+fh-w->RPort->TxBaseline) ;
        Text(w->RPort, msg, strlen(msg)) ;
        top += fh ;

        for ( i = 0 ; i < 4 ; i ++ )
        {
          joy    = ReadJoyPort(i) ;
          action = joytype(joy) ;
          
          sprintf(msg, "ReadJoyPort(%ld)     0x%08lx %s", i, joy, action) ;
          SetAPen(w->RPort, 1) ;
          SetBPen(w->RPort, 0) ;
          SetDrMd(w->RPort, JAM2) ;
          Move(w->RPort, left, top+fh-w->RPort->TxBaseline+8) ;
          Text(w->RPort, msg, strlen(msg)) ;

          joyaction(w->RPort, left+TextLength(w->RPort, msg, strlen(msg)), top, 2*fh, i, joy) ;

          top += 2*fh + 4 ;
        }

        Delay(10) ;
        /*WaitPort(w->UserPort) ;*/

        imsg = (struct IntuiMessage *)GetMsg(w->UserPort) ;
        if (imsg != NULL)
        {
          switch(imsg->Class)
          {
            case IDCMP_CLOSEWINDOW :
            {
              loop = FALSE ;
              break ;
            }
          }
          ReplyMsg(&imsg->ExecMessage) ;
        }
      } while (loop) ;
    }
    else
    {
      SetAPen(w->RPort, 1) ;
      Move(w->RPort, 10, 10) ;
      strcpy(msg, "lowlevel.library is missing") ;
      Text(w->RPort, msg, strlen(msg)) ;
      Delay(100) ; 
    }
    CloseWindow(w) ;
  }
  closelibs() ;
}
