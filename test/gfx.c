#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <aros/oldprograms.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/regions.h> 
#include <stdio.h>

struct NewWindow MyWin =
  {
    20,20,300,200,-1,-1,IDCMP_CLOSEWINDOW|/*IDCMP_DELTAMOVE|*/IDCMP_MOUSEMOVE,
    WINDOWCLOSE|WINDOWDRAG|WINDOWDEPTH|WINDOWSIZING|WFLG_SMART_REFRESH|WFLG_REPORTMOUSE,
    NULL,NULL,(char *)"Testwindow",
    NULL,NULL,0,0,0,0,WBENCHSCREEN 
  };

struct GfxBase * GfxBase;
struct Library * LayersBase;

void installClipRegion(struct Window * w)
{
  int width, height;
  struct Rectangle Rect;
  ULONG x,y,line;
  struct Region * R = NewRegion();
  printf("Width of ClipRegion Rectangles: ");
  scanf("%i",&width);
  printf("Height of ClipRegion Rectangles: ");
  scanf("%i",&height);
  
  if (height == 0 || width == 0)
    return;
  y = 0;
  line = 0;
  while (y < w->Height)
  {
    x = (line & 1) * width;
    while (x < w->Width)
    {
      Rect.MinX = x;
      Rect.MinY = y;
      Rect.MaxX = x+width-1;
      Rect.MaxY = y+height-1;
      OrRectRegion(R,&Rect);
    
      x += (2*width);
    }
    y += height;
    line ++;
  }
  
  InstallClipRegion(w->WLayer, R);
  
}
  
void uninstallClipRegion(struct Window * w)
{
  struct Region * R = InstallClipRegion(w->WLayer, NULL);
  if (NULL != R)
    DisposeRegion(R);
}

int main(void)
{
  LayersBase = OpenLibrary("layers.library",0);
  GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0);
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
        installClipRegion(TheDude);
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

/**/
        AreaMove(rp,10,20);
        AreaDraw(rp,110,30);
        AreaDraw(rp,110,100);
        AreaDraw(rp,10,50);
        AreaEnd(rp);
/**/
/*
        AreaEllipse(rp, 110, 30, 50, 20);
        AreaEnd(rp);        
*/
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
          if (IDCMP_MOUSEMOVE == Msg->Class)
          {
            printf("Received a delta move message! (%i,%i)\n",Msg->MouseX,Msg->MouseY);
          }
          ReplyMsg((struct Message *)Msg);
        }
        uninstallClipRegion(TheDude);
        CloseWindow(TheDude);
      }
      CloseLibrary((struct Library *)IntuitionBase);
    }
    CloseLibrary((struct Library *)GfxBase);
  }
  
    return 0;
}
