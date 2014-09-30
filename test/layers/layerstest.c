/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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
struct GfxBase *GfxBase;
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
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
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
	  CloseLibrary((struct Library *)GfxBase);
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
      DeleteLayer(0, layers[i]);
      if (layers[i]->SuperBitMap)
        FreeBitMap(layers[i]->SuperBitMap);
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
      if (s=='y' || s=='Y')
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
      if (s=='y' || s=='Y')
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
    DeleteLayer(0, layers[i]);
    if (layers[i]->SuperBitMap)
      FreeBitMap(layers[i]->SuperBitMap);
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

void sizelayer(void)
{
  int i,dx,dy;
  printf("Resize layer with id: ");
  scanf("%d",&i);
  printf("delta width: ");
  scanf("%d",&dx);
  printf("delta height: ");
  scanf("%d",&dy);
  if (layers[i])
  {
    SizeLayer(0, layers[i], dx, dy);
    printf("Resized layer with id %d.\n",i);
  }
  else
  {
    printf("No layer with id %d\n",i);
  }
}

void scrolllayer(void)
{
  int i,dx,dy;
  printf("Scroll layer with id: ");
  scanf("%d",&i);
  if (NULL == layers[i])
  {
    printf("No layer with id %d\n",i);
  }

  printf("delta x: ");
  scanf("%d",&dx);
  printf("delta y: ");
  scanf("%d",&dy);

  if (0 == (layers[i]->Flags & LAYERSUPER))
    printf("This is a non-superbitmapped layer.\n");
  else
    printf("This is a superbitmapped layer.\n");
 
  ScrollLayer(0, layers[i], dx, dy);
  printf("Scrolled layer with id %d.\n",i);

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
                                 95,
                                 LAYERSMART,
                                 NULL);
  
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 50,
                                 50,
                                 150,
                                 150,
                                 LAYERSMART,
                                 NULL);

  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 70,
                                 130,
                                 140,
                                 190,
                                 LAYERSMART,
                                 NULL);

  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo,
                                 screen->RastPort.BitMap,
                                 120,
                                 120,
                                 150,
                                 160,
                                 LAYERSMART,
                                 NULL);

  frame(layers[i]);
}

void GenerateLayers3(void)
{
  int i;
  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 110,
                                 110,
                                 200,
                                 195,
                                 LAYERSMART,
                                 NULL);
  
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 150,
                                 150,
                                 190,
                                 190,
                                 LAYERSMART,
                                 NULL);

  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 70,
                                 130,
                                 140,
                                 190,
                                 LAYERSMART,
                                 NULL);
  frame(layers[i]);

  i = unusedlayer();
  layers[i] = CreateBehindLayer(&screen->LayerInfo,
                                 screen->RastPort.BitMap,
                                 120,
                                 120,
                                 150,
                                 160,
                                 LAYERSMART,
                                 NULL);

  frame(layers[i]);
}

void GenerateLayers4(void)
{
  int i;
  int c = 0;
  while (c < 5)
  {
    i = unusedlayer();
    if (-1 == i)
      return;
    layers[i] = CreateBehindLayer(&screen->LayerInfo, 
                                   screen->RastPort.BitMap,
                                   10+c*10,
                                   10+c*10,
                                   100+c*10,
                                   100+c*10,
                                   LAYERSMART,
                                   NULL);
    frame(layers[i]);
    c++;
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
                                 LAYERSMART,
                                 NULL);
  
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


void DemoD()
{
  int i;
  int c;
  struct BitMap * sb;
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

  printf("Creating 4 superbitmap layers\n");

  sb = AllocBitMap(91,91,1,BMF_CLEAR,NULL);

  layers[0] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 10,
                                 10,
                                 100,
                                 100,
                                 LAYERSMART|LAYERSUPER,
                                 sb);
  
  frame(layers[0]);

  sb = AllocBitMap(71,71,1,BMF_CLEAR,NULL);
  layers[1] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 50,
                                 50,
                                 120,
                                 120,
                                 LAYERSMART|LAYERSUPER,
                                 sb);

  frame(layers[1]);

  sb = AllocBitMap(71,61,1,BMF_CLEAR,NULL);
  layers[2] = CreateUpfrontLayer(&screen->LayerInfo, 
                                 screen->RastPort.BitMap,
                                 70,
                                 30,
                                 140,
                                 90,
                                 LAYERSMART|LAYERSUPER,
                                 sb);
  frame(layers[2]);

  sb = AllocBitMap(131,41,1,BMF_CLEAR,NULL);
  layers[3] = CreateUpfrontLayer(&screen->LayerInfo,
                                 screen->RastPort.BitMap,
                                 20,
                                 20,
                                 150,
                                 60,
                                 LAYERSMART|LAYERSUPER,
                                 sb);

  frame(layers[3]);

  printf("Resizing layer 4\n");
  Delay(30);
  SizeLayer(0, layers[3], -80, -10);
  /* this leaves width = 51, height = 31 */
  printf("Scrolling layer 4\n");
  Delay(30);
  c = 0;
  while (c < 80)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[3], 1, 0);
  }
  
  c = 0;
  while (c < 10)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[3], 0 ,1);
  }

  c = 0;
  while (c < 10)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[3], -8 ,-1);
  }
  
  printf("Resizing layer 4 to its full size\n");
  Delay(30);
  SizeLayer(0, layers[3], 80, 10);
  
  printf("Resizing layer 1\n");
  SizeLayer(0, layers[0], -50, -50);

  /* this leaves width = 41, height = 41 */
  printf("Scrolling layer 1\n");
  Delay(30);
  c = 0;
  while (c < 41)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[0], 1, 0);
  }
  
  c = 0;
  while (c < 41)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[0], 0 ,1);
  }
  
  c = 0;
  while (c < 41)
  {
    c++;
    Delay(5);
    ScrollLayer(0, layers[0], -1 ,-1);
  }

  printf("Resizing layer 1 to its full size\n");
  Delay(30);
  SizeLayer(0, layers[0], 50, 50);

  printf("Shuffling layers...\n");
  UpfrontLayer(0, layers[0]);
  Delay(20);
  UpfrontLayer(0, layers[1]);
  Delay(20);
  UpfrontLayer(0, layers[2]);
  Delay(20);
  UpfrontLayer(0, layers[3]);
  Delay(20);
  BehindLayer(0, layers[2]);
  Delay(20);
  BehindLayer(0,layers[1]);
  Delay(20);
  BehindLayer(0,layers[0]);
  printf("Inviting a few smart friends...\n");
  GenerateLayers3();
  printf("Moving the layers...\n");
  c = 0;
  while (c < 40)
  {
    MoveLayer(0, layers[4], -1, -1);
    MoveLayer(0, layers[0], 1,2);
    MoveLayer(0, layers[2], 2,1);
    MoveLayer(0, layers[5], -2, 0);
    c++;
  }

  c = 0;
  while (c < 30)
  {
    MoveLayer(0, layers[4], -1, -1);
    MoveLayer(0, layers[5], -1, 0);
    MoveLayer(0, layers[3], 3, 2);
    MoveSizeLayer(layers[2], 2, -1, -1, -1);
    c++;
  }

  c = 0;
  while (c < 30)
  {
    MoveSizeLayer(layers[2], -2, 1, 1 ,1);
    MoveSizeLayer(layers[1],  2, 0, -1 ,-1);
    MoveLayer(0, layers[5], 2, 1);
    c++;
  }

  c = 0;
  while (c < 30)
  {
    MoveSizeLayer(layers[1], -1, -1 ,1 , 1);
    c++;
  }
  
  printf("Deleting the layers...\n");
  i = 0;
  while (i < 8)
  {
    while ((layers[i]->bounds.MaxX - layers[i]->bounds.MinX) >= 1 &&
           (layers[i]->bounds.MaxY - layers[i]->bounds.MinY) >= 1)
    {
      MoveSizeLayer(layers[i], 2, 0, -1, -1);
    }
    DeleteLayer(0, layers[i]);
    layers[i] = NULL;
    i++;
  }  
}

void DumpCliprects(void)
{
  int i;
  struct ClipRect * CR;
  int c;
  
  printf("Dump ClipRects of which layer? ");
  scanf("%i",&i);
  if (i < 0 || i > 10 || NULL == layers[i])
  {
    printf("No such layer!\n");
    return;
  }
  
  printf("Layer at %px is a ",layers[i]);
  if (0 != (layers[i]->Flags & LAYERSIMPLE))
    printf("simple layer\n");
  else if (0 != (layers[i]->Flags & LAYERSUPER))
    printf("superbitmap layer\n");
  else
    printf("smart layer\n");
  
  c = 1;
  CR = layers[i]->ClipRect;
  while (NULL != CR)
  {
    printf("CR #%i\n",c);
    
    printf("Bounds: (%i,%i)-(%i,%i)\n",CR->bounds.MinX,CR->bounds.MinY,CR->bounds.MaxX,CR->bounds.MaxY);
    if (NULL != CR->lobs)
      printf("This cliprect is hidden by layer %px !\n",CR->lobs);
    else
      printf("This cliprect is visible!\n");
    c++;
    CR=CR->Next;
  }  

  printf("-------------------------------------\n");  
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
        printf("movelayer [ml] movesizelayer [msl] sizelayer [sl] scrollayer [scl]\n");
        printf("motion [mot] DumpCliprects [dc] \n");
        printf("Frame [F]  DemoA DemoB DemoC DemoD\n");
        printf("Generate a few layers: [gl1,gl2,gl3,gl4]\n");
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
    else if (!strcmp(buf, "sizelayer") || !strcmp(buf, "sl")) 
    {
      sizelayer();
    }     
    else if (!strcmp(buf, "scrolllayer") || !strcmp(buf, "scl")) 
    {
      scrolllayer();
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
    else if (!strcmp(buf, "DemoD")) 
    {
      DemoD();
    } 
    else if (!strcmp(buf, "gl1")) 
    {
      GenerateLayers1();
    } 
    else if (!strcmp(buf, "gl2")) 
    {
      GenerateLayers2();
    } 
    else if (!strcmp(buf, "gl3")) 
    {
      GenerateLayers3();
    } 
    else if (!strcmp(buf, "gl4")) 
    {
      GenerateLayers4();
    } 
    else if (!strcmp(buf, "dc")) 
    {
      DumpCliprects();
    } 
    else
      printf("Unknown command. Try 'help'.\n");
  }
}
