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

#define GFX_SYSTEM X11  /* ord HIDD */

struct IntuitionBase *IntuitionBase;
struct Library *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;
struct Screen *screen;
struct Window * window;

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
		doall();
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


struct Screen * openscreen(void)
{
  struct Screen * screen;

  screen = OpenScreenTags(NULL,
                          SA_Height, 200,
                          SA_Width, 300,
                          TAG_END);

  Draw(&screen->RastPort, 100, 100);

  return screen;
}

void closescreen(struct Screen * screen)
{
  CloseScreen(screen);
}

struct Layer *layers[10];

void freelayers(void)
{
  int i;
  for (i=0; i < 10; i++) 
  {
    if (NULL != layers[i])
    {
      if (layers[i]->SuperBitMap)
        FreeBitMap(layers[i]->SuperBitMap);
      DeleteLayer(0, layers[i]);
    }
  }
}

int unusedlayer(void)
{
  int i;
  for (i =0; i < 10; i++)
  {
    if (NULL == layers[i])
      return i;
  }
  return -1;
}

void createupfrontlayer(void)
{
  int x0,y0,x1,y1;
  int i;
  char c,s;
  LONG flags = 0;
  struct BitMap * sb = NULL;
  for (i = 0; i < 10; i++)
  {
    if (layers[i] == NULL)
      break;
  }
  if (i < 10) 
  {
      printf("Backdroplayer [y/N]: ");
      scanf("%c", &c);
      printf("SuperBitMap [y/N]: ");
      scanf("%c", &s);
      printf("x0: ");
      scanf("%d", &x0);
      printf("y0: ");
      scanf("%d", &y0);
      printf("x1: ");
      scanf("%d", &x1);
      printf("y1: ");
      scanf("%d", &y1);
    
      if (c=='y' || c=='Y')
      {
        printf("Generating a backdrop layer.\n");
        flags |= LAYERBACKDROP;
      }
      if (c=='s' || c=='Y')
      {
        printf("Generating a superbitmap layer.\n");
        flags |= LAYERSUPER;
        sb = AllocBitMap(x1-x0+1,y1-y0+1,1,BMF_CLEAR,NULL);
      }
      
      flags |= LAYERSMART;

      layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                     screen->RastPort.BitMap,
                                     x0,
                                     y0,
                                     x1,
                                     y1,
                                     flags,
                                     sb);
      if (layers[i])
        printf("Created layer with ID %d\n",i);
      else
        printf("Couldn't create layer. No more memory (?).\n");
  }
  else
    printf("No more layers possible!\n");
}

void createbehindlayer(void)
{
  int x0,y0,x1,y1;
  int i;
  char c, s;
  LONG flags = 0;
  struct BitMap * sb = NULL;
  for (i = 0; i < 10; i++)
  {
    if (layers[i] == NULL)
      break;
  }
  if (i < 10) 
  {
      printf("Backdroplayer [y/N]: ");
      scanf("%c", &c);
      printf("SuperBitMap [y/N]: ");
      scanf("%c", &s);
      printf("x0: ");
      scanf("%d", &x0);
      printf("y0: ");
      scanf("%d", &y0);
      printf("x1: ");
      scanf("%d", &x1);
      printf("y1: ");
      scanf("%d", &y1);

      if (c=='y' || c=='Y')
      {
        printf("Generating a backdrop layer.\n");
        flags |= LAYERBACKDROP;
      }
      if (c=='s' || c=='Y')
      {
        printf("Generating a superbitmap layer.\n");
        flags |= LAYERSUPER;
        sb = AllocBitMap(x1-x0+1,y1-y0+1,1,BMF_CLEAR,NULL);
      }
      
      flags |= LAYERSMART;
    
      layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                     screen->RastPort.BitMap,
                                     x0,
                                     y0,
                                     x1,
                                     y1,
                                     flags,
                                     sb);
      if (layers[i])
         printf("Created layer with ID %d\n",i);
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
    if (layers[i]->SuperBitMap)
      FreeBitMap(layers[i]->SuperBitMap);
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

void behindlayer(void)
{
  int i;
  printf("Behind layer with id: ");
  scanf("%d", &i);
  if (layers[i]) {
    BehindLayer(0, layers[i]);
    printf("Moved layer with id %d behind\n",i);
  } 
  else 
  {
    printf("No layer with id %d\n",i);
  }
}

void movelayerinfrontof(void)
{
  int i1,i2;
  printf("Move layer with id: ");
  scanf("%d", &i1);
  printf("in front of layer with id: ");
  scanf("%d", &i2);
  if (layers[i1] && layers[i2]) {
    MoveLayerInFrontOf(layers[i1], layers[i2]);
    printf("Moved layer with id %d in front of layer with id %d\n",i1,i2);
  } 
  else 
  {
    printf("No layer with id %d or id %d\n",i1,i2);
  }
}

void movelayer(void)
{
  int i,dx,dy;
  printf("Move layer with id: ");
  scanf("%d",&i);
  printf("delta x: ");
  scanf("%d",&dx);
  printf("delta y: ");
  scanf("%d",&dy);
  if (layers[i])
  {
    MoveLayer(0, layers[i], dx, dy);
    printf("Moved layer with id %d to new position.\n",i);
  }
  else
  {
    printf("No layer with id %d\n",i);
  }
}

void movesizelayer(void)
{
  int i,dx,dy,dw,dh;
  printf("Move and size layer with id: ");
  scanf("%d",&i);
  printf("delta x: ");
  scanf("%d",&dx);
  printf("delta y: ");
  scanf("%d",&dy);
  printf("delta width: ");
  scanf("%d",&dw);
  printf("delta height: ");
  scanf("%d",&dh);
  if (layers[i])
  {
    MoveSizeLayer(layers[i], dx, dy, dw, dh);
    printf("Moved and sized layer with id %d.\n",i);
  }
  else
  {
    printf("No layer with id %d\n",i);
  }
}

void motion(void)
{
  int i,dx,dy,iter;
  printf("Move layer with id: ");
  scanf("%d",&i);
  printf("delta x: ");
  scanf("%d",&dx);
  printf("delta y: ");
  scanf("%d",&dy);
  printf("iterations: ");
  scanf("%d",&iter);
  if (layers[i])
  {
    while (iter > 0 && (TRUE == MoveLayer(0, layers[i],dx,dy)))
      iter--;
    printf("Moved layer with id %d to new position.\n",i);
  }
  else
  {
    printf("No layer with id %d\n",i);
  }
}

/* Draw a simple frame around a layer */
void frame(struct Layer * layer)
{
    int c, width = layer->bounds.MaxX-layer->bounds.MinX,i=0;
    SetAPen(layer->rp, 1);
    Move(layer->rp, 0,0);
    Draw(layer->rp, layer->bounds.MaxX - layer->bounds.MinX, 0);
    Draw(layer->rp, layer->bounds.MaxX - layer->bounds.MinX, 
                    layer->bounds.MaxY - layer->bounds.MinY);
    Draw(layer->rp, 0, layer->bounds.MaxY - layer->bounds.MinY);
    Draw(layer->rp, 0, 0);
    Draw(layer->rp, layer->bounds.MaxX - layer->bounds.MinX, 
                    layer->bounds.MaxY - layer->bounds.MinY);
   
    for (c=0; c <= width; c= c+(width&0x0f)+i )
    {
      i+=2;
      Move(layer->rp,c,0);
      Draw(layer->rp,c,layer->bounds.MaxY - layer->bounds.MinY);
    } 
}

void Frame(void)
{
  int i;
  printf("Framing layer with id: ");
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

void GenerateLayers1(void)
{
  int i;
  i = unusedlayer();
  if (i==-1) return;
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 10,
                                 10,
                                 100,
                                 100,
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);
  
  i = unusedlayer();
  if (i==-1) return;
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 50,
                                 50,
                                 150,
                                 150,
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);
  
  i = unusedlayer();
  if (i==-1) return;
  layers[i] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 20,
                                 20,
                                 120,
                                 70,
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);
}

void GenerateLayers2(void)
{
  int i;
  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 10,
                                 10,
                                 100,
                                 100,
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 50,
                                 50,
                                 120,
                                 120,
                                 LAYERSMART,
                                 NULL);

  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 70,
                                 30,
                                 140,
                                 90,
                                 LAYERSMART,
                                 NULL);
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo,
                                 screen->RastPort.BitMap,
                                 20,
                                 20,
                                 150,
                                 60,
                                 LAYERSMART,
                                 NULL);

  /*
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);
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
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
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
                                 LAYERSMART,
                                 NULL);
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
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
                                 LAYERSMART,
                                 NULL);
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
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
                                 LAYERSMART,
                                 NULL);
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
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

void DemoB(void)
{
#undef DELAYTIME
#define DELAYTIME 100
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
  printf("Creating behind-layer %i.\n",i);
  Delay(DELAYTIME);
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 10,
                                 10,
                                 100,
                                 100,
                                 LAYERSMART,
                                 NULL);
  
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);

  printf("Creating behind-layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 50,
                                 50,
                                 120,
                                 120,
                                 LAYERSMART,
                                 NULL);

  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif  
  frame(layers[i]);

  printf("Creating behind-layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 70,
                                 30,
                                 140,
                                 90,
                                 LAYERSMART,
                                 NULL);
  /* 
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);


  printf("Creating behind-layer %i.\n",++i);
  Delay(DELAYTIME);
  layers[i] = CreateBehindLayer(&screen->LayerInfo,
                                 screen->RastPort.BitMap,
                                 20,
                                 20,
                                 150,
                                 60,
                                 LAYERSMART,
                                 NULL);

  /*
    CHEAT!!!
    Still have to copy the X-Window info manually. It is found
    in longreserved[0] of the RastPort of a window (should be found
    in a screen anyway)
  */
#if 0
  layers[i]->rp->longreserved[0] = window->RPort->longreserved[0];
#endif
  frame(layers[i]);

  i = 2;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);

  printf("Moving layer %i in front of layer %i\n",3,0);
  Delay(DELAYTIME);
  MoveLayerInFrontOf(layers[3], layers[0]);

  printf("Moving layer %i in front of layer %i\n",0,3);
  Delay(DELAYTIME);
  MoveLayerInFrontOf(layers[0], layers[3]);

  i = 1;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);

  i = 0;
  printf("Moving layer %i in front of all other layers.\n",i);
  Delay(DELAYTIME);
  UpfrontLayer(0, layers[i]);

  i = 1;
  printf("Moving layer %i behind all other layers.\n",i);
  Delay(DELAYTIME);
  BehindLayer(0, layers[i]);

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

void DemoC()
{
  int i;
  printf("Generating a few layers\n");
  GenerateLayers2();
  if (layers[1])
  {
    printf("Moving layer 1...\n");
    Delay(30);
    i = 50;
    while (i>0 && (TRUE == MoveLayer(0, layers[1], 1,1)))
        i--;
  }
  if (layers[2])
  {
    printf("Moving layer 2...\n");
    Delay(30);
    i = 50;
    while (i>0 && (TRUE == MoveLayer(0, layers[2], 2, -1)))
      i--;
  }  
  if (layers[3])
  {
    printf("Moving layer 3...\n");
    Delay(30);
    i = 50;
    while (i>0 && TRUE == MoveLayer(0, layers[3], 1, 3))
      i--;
  }
  if (layers[0])
  {
    printf("Moving layer 0...\n");
    Delay(30);
    i = 150;
    while (i>0 && TRUE == MoveLayer(0, layers[0], 0 ,1))
      i--;
  }
  printf("Deleting all visible layers!\n");
  i = 0;
  while (i < 10)
  {
    if (layers[i])
      DeleteLayer(0, layers[i]);
    layers[i] = NULL;
    i++;
  }    
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
        printf("quit help createupfrontlayer [cul] createbehindlayer [cbl] deletelayer [dl]\n");
        printf("behindlayerupfrontlayer [ul] movelayerinfrontof [mlio]\n");
        printf("movelayer [ml] movesizelayer [msl] motion [mot] \n");
        printf("Frame [F]  DemoA DemoB DemoC\n");
    } 
    else if (!strcmp(buf, "createupfrontlayer") || !strcmp(buf, "cul")) 
    {
      createupfrontlayer();
    } 
    else if (!strcmp(buf, "createbehindlayer") || !strcmp(buf, "cbl")) 
    {
      createbehindlayer();
    } 
    else if (!strcmp(buf, "deletelayer") || !strcmp(buf, "dl")) 
    {
      deletelayer();
    } 
    else if (!strcmp(buf, "behindlayer") || !strcmp(buf, "bl")) 
    {
      behindlayer();
    } 
    else if (!strcmp(buf, "upfrontlayer") || !strcmp(buf, "ul")) 
    {
      upfrontlayer();
    } 
    else if (!strcmp(buf, "movelayerinfrontof") || !strcmp(buf, "mlio")) 
    {
      movelayerinfrontof();
    }
    else if (!strcmp(buf, "movelayer") || !strcmp(buf, "ml")) 
    {
      movelayer();
    }     
    else if (!strcmp(buf, "movesizelayer") || !strcmp(buf, "msl")) 
    {
      movesizelayer();
    }     
    else if (!strcmp(buf, "motion") || !strcmp(buf, "mot")) 
    {
      motion();
    }     
    else if (!strcmp(buf, "Frame")  || !strcmp(buf, "F")) 
    {
      Frame();
    } 
    else if (!strcmp(buf, "DemoA")) 
    {
      DemoA();
    } 
    else if (!strcmp(buf, "DemoB")) 
    {
      DemoB();
    } 
    else if (!strcmp(buf, "DemoC")) 
    {
      DemoC();
    } 
    else if (!strcmp(buf, "gl1")) 
    {
      GenerateLayers1();
    } 
    else if (!strcmp(buf, "gl2")) 
    {
      GenerateLayers2();
    } 
    else
      printf("Unknown command. Try 'help'.\n");
  }
}
