#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <aros/oldprograms.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h> 

struct NewWindow MyWin =
  {
    20,20,300,200,-1,-1,CLOSEWINDOW,
    WINDOWCLOSE|WINDOWDRAG|WINDOWDEPTH|WFLG_GIMMEZEROZERO,
    NULL,NULL,(char *)"Testwindow",
    NULL,NULL,0,0,0,0,WBENCHSCREEN 
  };
  
void main(void)
{
  struct GfxBase * GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0);
  if (NULL != GfxBase)
  {
    struct IntuitionBase * IntuitionBase =
       (struct IntuitionBase *)OpenLibrary("intuition.library",0);
    if (NULL != IntuitionBase)
    {
      struct Window * TheDude = OpenWindow(&MyWin);
      if (NULL != TheDude)
      {
        struct RastPort * rp = TheDude->RPort;
        struct TmpRas tmpras;
        struct IntuiMessage * Msg;
        UWORD areabuffer[250];
        char c;
        struct AreaInfo myAreaInfo;
        InitArea(&myAreaInfo, &areabuffer[0], 50);
        rp->AreaInfo = &myAreaInfo;
        InitTmpRas(&tmpras, AllocRaster(320,200),RASSIZE(320,200));
        rp->TmpRas = &tmpras;
        
        /* Let's draw something */
        SetAPen(rp,1);
        SetOutlinePen(rp,3);
        
/*
        AreaMove(rp,10,20);
        AreaDraw(rp,110,30);
        AreaDraw(rp,110,100);
        AreaDraw(rp,10,110);
        AreaEnd(rp);
*/

        AreaMove(rp,10,20);
        AreaDraw(rp,110,30);
        AreaDraw(rp,110,100);
        AreaDraw(rp,10,50);
        AreaEnd(rp);

        AreaEllipse(rp, 110, 30, 50, 20);
        AreaEnd(rp);        
/*
        Move(rp,0,0);
        Draw(rp,0,100);
        Draw(rp,100,110);
        Draw(rp,100,0);
        Draw(rp,0,0);
        SetAPen(rp, 1);
        Flood(rp,0,50,50);
*/
/*        
        ScrollRaster(&IntuitionBase->ActiveScreen->RastPort,
                     -1,
                     -1,
                     10,
                     10,
                     100,
                     100);
*/
        printf("press a key and hit return!");
        scanf("%c",&c);
        ScrollRaster(rp,
                     -10,
                     -10,
                     10,
                     10,
                     100,
                     100);

    
        while (TRUE)
        {               
          WaitPort(TheDude->UserPort);
          Msg = (struct IntuiMessage *)GetMsg(TheDude->UserPort);
          if (IDCMP_CLOSEWINDOW == Msg->Class)
            break;
          ReplyMsg((struct Message *)Msg);
        }
        CloseWindow(TheDude);
      }
      CloseLibrary((struct Library *)IntuitionBase);
    }
    CloseLibrary((struct Library *)GfxBase);
  }
}
