#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <aros/asmcall.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>

#define GFX_SYSTEM X11   /* ord HIDD */
 
struct IntuitionBase *IntuitionBase;
struct Library *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;
struct Screen *screen;

#if 0
struct Window * window;
#endif

static const char version[] = "$VER: regiontest 41.1 (14.3.1997)\n";

void init(void);
void restore(void);
void doall(void);
struct Screen * openscreen(void);
void closescreen(struct Screen * screen); 

APTR BltBitMapPtr;

int main(int argc, char **argv)
{
    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = OpenLibrary("graphics.library", 0))) 
        {
	  if ((LayersBase = OpenLibrary("layers.library", 0))) 
          {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
              if ((screen = openscreen())) 
              {
#if 0
                init();
#endif
		doall();
#if 0
                restore();
#endif
		closescreen(screen);
	      }
              CloseLibrary((struct Library *)DOSBase);
	    }
            CloseLibrary(LayersBase);
	  }
	  CloseLibrary(GfxBase);
	}
	CloseLibrary((struct Library *) IntuitionBase);
    }
    return 0;
} /* main */


#if 0
/*
  CHEAT!!!
  BltBitMap within AROS doesn't do it a the moment.
 */

AROS_UFH11(ULONG, MyBltBitMap,
  AROS_UFHA(struct BitMap *, SrcBitMap, A0),
  AROS_UFHA(WORD, SrcX, D0),
  AROS_UFHA(WORD, SrcY, D1),
  AROS_UFHA(struct BitMap *, DstBitMap, A1),
  AROS_UFHA(WORD, DstX, D2),
  AROS_UFHA(WORD, DstY, D3),
  AROS_UFHA(WORD, SizeX, D4),
  AROS_UFHA(WORD, SizeY, D5),
  AROS_UFHA(UBYTE, Minterm, D6),
  AROS_UFHA(UBYTE, Mask, D7),
  AROS_UFHA(UWORD *, TempA, A2))
{
  struct RastPort SrcRastPort;
  struct RastPort DstRastPort;
  WORD x,y;
  UBYTE pen;


  InitRastPort(&SrcRastPort);
  InitRastPort(&DstRastPort);
  SrcRastPort.BitMap = SrcBitMap;
  DstRastPort.BitMap = DstBitMap;
  

  /* 
    Is the destination BitMap the one of the two (!!) bitmapstructures
    that belongs to my screen?
    Actually in this case as there's only one Screen it would be sufficient
    to just test the BitMap->Flag for BMF_AROS_DISPLAYED
  */

  if (DstBitMap == &screen->BitMap  || DstBitMap == screen->RastPort.BitMap)
  {
    /* 
       Then we have to copy the longreserved[0]-entry of the screen to the 
       DstRastPort as there are the X-Window handle etc.
     */ 
    DstRastPort.longreserved[0] = screen->RastPort.longreserved[0];
  }

  switch (Minterm)
  {
    case 0:
      /* all we do is erase the rectangle in the destination rastport */
       SetAPen(&DstRastPort, 0);
       for (x = DstX; x < (SizeX+DstX);  x++)
         for (y = DstY; y < (SizeY+DstY); y++)
           WritePixel(&DstRastPort, x, y); 
    break;

    default:
      /* Plain vanilla copy from SrcBitMap to DstBitMap */
       for (x = 0; x < SizeX;  x++)
         for (y = 0; y < SizeY; y++)
	 {
           pen =  ReadPixel(&SrcRastPort, x+SrcX, y+SrcY);
           SetAPen(&DstRastPort, pen);
           WritePixel(&DstRastPort, x+DstX, y+DstY);
	 }         
    break;
  }
  return 0;
}


void init(void)
{
  /* 
     CHEAT!!!
     Unfortunately I have to redirect BltBitMap to my own function
     The one we currently have just doesn't do it!
   */
  BltBitMapPtr = SetFunction(GfxBase, 5 * (-LIB_VECTSIZE) ,&MyBltBitMap);
}

void restore(void)
{
  BltBitMapPtr = SetFunction(GfxBase, 5 * (-LIB_VECTSIZE) ,BltBitMapPtr);
}

#endif

struct Screen * openscreen(void)
{
  struct Screen * screen;
  /* 
    CHEAT!!!
    OpenScreen() unfortunately does not open any screen so far.
    So we have to put all the necessary info from the windows
    rastport into the screen's rastport so we can draw into the screen!
   */
  screen = OpenScreenTags(NULL,
                          SA_Height, 600,
                          SA_Width, 800,
                          TAG_END);

#if 0
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY,
                          WA_Height, 600,
                          WA_Width, 800,
                          TAG_END);


  screen->RastPort.longreserved[0] = window->RPort->longreserved[0];
#endif

  Draw(&screen->RastPort, 100, 100);

  return screen;
}

void closescreen(struct Screen * screen)
{
#if 0
  CloseWindow(window);
#endif  
  CloseScreen(screen);
}

struct Layer *layers[10];

void freelayers(void)
{
  int i;
  for (i=0; i < 10; i++) 
  {
    if (NULL != layers[i])
      DeleteLayer(0, layers[i]);
  }
}

void createupfrontlayer(void)
{
  int x0,y0,x1,y1;
  int i;
  for (i = 0; i < 10; i++)
  {
    if (layers[i] == NULL)
      break;
  }
  if (i < 10) 
  {
      printf("x0: ");
      scanf("%d", &x0);
      printf("y0: ");
      scanf("%d", &y0);
      printf("x1: ");
      scanf("%d", &x1);
      printf("y1: ");
      scanf("%d", &y1);
    
      layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                     screen->RastPort.BitMap,
                                     x0,
                                     y0,
                                     x1,
                                     y1,
                                     0, /* LAYER_SMART*/
                                     NULL);
      if (layers[i])
      {
        /* 
           CHEAT!!!
           Still have to copy the X-Window info manually. It is found
           in longreserved[0] of the RastPort of a window (should be found
           in a screen anyway)
	 */
#if 0
        layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif	
        printf("Created layer with ID %d\n",i);
      }
      else
        printf("Couldn't create layer. No more memory (?).\n");
  }
  else
    printf("No more layers possible!\n");
}

void deletelayer(void)
{
  int i;
  printf("Delete layer with id: ");
  scanf("%d", &i);
  if (layers[i]) 
  {
    DeleteLayer(0, layers[i]);
    printf("Deleted layer with id %d\n",i);
    layers[i] = NULL;
  } 
  else 
  {
    printf("No layer with id %d\n",i);
  }
}

void upfrontlayer(void)
{
  int i;
  printf("Upfront layer with id: ");
  scanf("%d", &i);
  if (layers[i]) {
    UpfrontLayer(0, layers[i]);
    printf("Moved layer with id %d upfront\n",i);
  } 
  else 
  {
    printf("No layer with id %d\n",i);
  }
}

/* Draw a simple frame around a layer */
void frame(struct Layer * layer)
{
    SetAPen(layer->rp, 1);
    Move(layer->rp, 0,0);
    Draw(layer->rp, layer->bounds.MaxX - layer->bounds.MinX, 0);
    Draw(layer->rp, layer->bounds.MaxX - layer->bounds.MinX, 
                    layer->bounds.MaxY - layer->bounds.MinY);
    Draw(layer->rp, 0, layer->bounds.MaxY - layer->bounds.MinY);
    Draw(layer->rp, 0, 0);
}

void Frame(void)
{
  int i;
  printf("PatternA into layer with id: ");
  scanf("%d", &i);
  if (layers[i]) 
  {
    /* Draw a Frame */
    frame(layers[i]);
  } 
  else 
  {
    printf("No layer with id %d\n",i);
  }
}

void PatternA(void)
{
  int i;
  printf("PatternA into layer with id: ");
  scanf("%d", &i);
  if (layers[i]) {

  } else {
    printf("No layer with id %d\n",i);
  }
}

void PatternB(void)
{
  int i;
  printf("PatternB into layer with id: ");
  scanf("%d", &i);
  if (layers[i]) {

  } else {
    printf("No layer with id %d\n",i);
  }
}

void DemoA(void)
{
#define DELAYTIME 50
  int i;
  printf("Deleting all previously generated layers...\n");
  for (i = 0; i < 10; i++) 
  {
    if (layers[i])
    {
      DeleteLayer(0, layers[i]);
      layers[i] = NULL;
    }
  }
  i = 0;

  printf("After each step there will be a short delay.\n");
  printf("Activate other X-window (Amiga Screen) and always press a key\nto see what is going on\n");
  printf("Creating layer %i.\n",i);
  Delay(DELAYTIME);
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 10,
                                 10,
                                 100,
                                 100,
                                 0, /* LAYER_SMART*/
                                 NULL);

#if 0
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];

#endif  

  frame(layers[i]);

  printf("Creating layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 20,
                                 5,
                                 30,
                                 150,
                                 0, // LAYER_SMART
                                 NULL);

#if 0
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  
  frame(layers[i]);

  printf("Creating layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 5,
                                 50,
                                 150,
                                 70,
                                 0, /* LAYER_SMART*/
                                 NULL);

#if 0
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  
  frame(layers[i]);

  printf("Creating layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 80,
                                 5,
                                 180,
                                 200,
                                 0, // LAYER_SMART
                                 NULL);

#if 0
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif

  frame(layers[i]); 
   
  i = 0;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 1;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 2;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 3;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  
  i = 0;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 2;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 2;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  i = 1;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);

  i = 2;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);
  
  for (i = 0; i < 10; i++)
    if (NULL != layers[i])
    {
      printf("Destroying layer %i.\n",i);
      Delay(DELAYTIME);
      DeleteLayer(0, layers[i]);
      layers[i] = NULL;
    }

  printf("End of demo .\n");
}

void doall(void)
{
  char buf[80];
  int i;
  for (i = 0; i < 10; i++)
    layers[i] = NULL;

  for (;;) 
  {
    printf("> ");
    fflush(stdout);
    scanf("%s", buf);
  
    if (!strcmp(buf,"quit")) 
    {
       freelayers();
       return;
    } 
    else if (!strcmp(buf, "help")) 
    {
        printf("quit help createupfrontlayer deletelayer upfrontlayer\n");
        printf("Frame PatternA PatternB DemoA\n");
    } 
    else if (!strcmp(buf, "createupfrontlayer")) 
    {
      createupfrontlayer();
    } 
    else if (!strcmp(buf, "deletelayer")) 
    {
      deletelayer();
    } 
    else if (!strcmp(buf, "upfrontlayer")) 
    {
      upfrontlayer();
    } 
    else if (!strcmp(buf, "Frame")) 
    {
      Frame();
    } 
    else if (!strcmp(buf, "PatternA")) 
    {
      PatternA();
    } 
    else if (!strcmp(buf, "PatternB")) 
    {
      PatternB();
    } 
    else if (!strcmp(buf, "DemoA")) 
    {
      DemoA();
    } 
    else
      printf("Unknown command. Try 'help'.\n");
  }
}
