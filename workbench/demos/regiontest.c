/* tab = 3 */
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Window *win;

static const char version[] __attribute__((used)) = "$VER: regiontest 41.1 (14.3.1997)\n";

void doall(void);

int main(int argc, char **argv)
{
    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) {
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) {
	    if ((win = OpenWindowTags(NULL,
				     WA_IDCMP,	IDCMP_RAWKEY,
				     WA_Height, 400,
				     WA_Width, 800,
				     TAG_END))) {
		doall();
		CloseWindow(win);
	    }
	    CloseLibrary((struct Library *)GfxBase);
	}
	CloseLibrary((struct Library *) IntuitionBase);
    }
    return 0;
}



struct Region *regions[10];

void freeregions(void)
{
    int i;
    for (i = 0; i < 10; i++) {
	if (regions[i])
	    DisposeRegion(regions[i]);
    }
}

void newregion(void)
{
    int i;
    for (i = 0; i < 10; i++)
	if (regions[i] == NULL)
	    break;
    if (i < 10) {
	if ((regions[i] = NewRegion())) {
	    printf("New Region created, id: %d\n", i);
	} else {
	    printf("Sorry, not enough mem\n");
	}
    } else {
	printf("Sorry, no more regions possible\n");
    }
}

void disposeregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush (stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    DisposeRegion(regions[i]);
	    regions[i] = NULL;
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void DrawRectangle(struct Rectangle *r, int offsetx, int offsety)
{
    Move(win->RPort, r->MinX + offsetx, r->MinY + offsety);
    Draw(win->RPort, r->MaxX + offsetx, r->MinY + offsety);
    Delay(10);
    Draw(win->RPort, r->MaxX + offsetx, r->MaxY + offsety);
    Delay(10);
    Draw(win->RPort, r->MinX + offsetx, r->MaxY + offsety);
    Delay(10);
    Draw(win->RPort, r->MinX + offsetx, r->MinY + offsety);
    Delay(10);
}

void FillRectangle(struct Rectangle *r, int offsetx, int offsety)
{
    RectFill(win->RPort, r->MinX + 1 + offsetx, r->MinY + 1 + offsety,
	     r->MaxX - 1 + offsetx, r->MaxY - 1 + offsety);
}

void showregions(void)
{
    int i;
    printf("The following Region id's are currently in use:\n");
    for (i = 0; i < 10; i++)
	if (regions[i]) {
	    printf("\t%d - bounds: (%d,%d) - (%d,%d)\n", i,
		   regions[i]->bounds.MinX, regions[i]->bounds.MinY,
		   regions[i]->bounds.MaxX, regions[i]->bounds.MaxY);
	    SetAPen(win->RPort, 3);
	    DrawRectangle(&regions[i]->bounds, 0, 0);
	}
}

void showrects(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    struct RegionRectangle *current;
	    for (current = regions[i]->RegionRectangle; current; current = current->Next) {
		printf("\tbounds: (%d,%d) - (%d,%d)\n", current->bounds.MinX,
		       current->bounds.MinY, current->bounds.MaxX, current->bounds.MaxY);
		SetAPen(win->RPort, 1);
		DrawRectangle(&current->bounds, regions[i]->bounds.MinX, regions[i]->bounds.MinY);
		SetAPen(win->RPort, 2);
		FillRectangle(&current->bounds, regions[i]->bounds.MinX, regions[i]->bounds.MinY);
		printf("press a key\n");
		//Wait(1L << win->UserPort->mp_SigBit);
	    }
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void getrectangle(struct Rectangle *rect)
{
    int val;
    printf("Rectangle left edge: ");
    scanf("%d", &val);
    rect->MinX = val;
    printf("Rectangle top edge: ");
    scanf("%d", &val);
    rect->MinY = val;
    printf("Rectangle right edge: ");
    scanf("%d", &val);
    rect->MaxX = val;
    printf("Rectangle lower edge: ");
    scanf("%d", &val);
    rect->MaxY = val;
}

void orrectregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    struct Rectangle rect;
	    getrectangle(&rect);
	    if (OrRectRegion(regions[i], &rect))
		printf("Done.\n");
	    else
		printf("Out of memory\n");
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void andrectregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    struct Rectangle rect;
	    getrectangle(&rect);
	    AndRectRegion(regions[i], &rect);
	    printf("Done.\n");
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void clearrectregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    struct Rectangle rect;
	    getrectangle(&rect);
	    if (ClearRectRegion(regions[i], &rect))
		printf("Done.\n");
	    else
		printf("Out of memory\n");
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void xorrectregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    struct Rectangle rect;
	    getrectangle(&rect);
	    if (XorRectRegion(regions[i], &rect))
		printf("Done.\n");
	    else
		printf("Out of memory\n");
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void clearregion(void)
{
    int i;
    printf("Enter Region id: ");
    fflush(stdout);
    scanf("%d", &i);
    if (i >= 0 && i < 10)
	if (regions[i]) {
	    ClearRegion(regions[i]);
	    printf("Done.\n");
	    return;
	}
    printf("Region with id %d does not exist\n", i);
}

void xorregionregion(void)
{
  int i1,i2;
  printf("Enter Region1 id: ");
  fflush(stdout);
  scanf("%d", &i1);
  printf("Enter Region2 (destination) id: ");
  fflush(stdout);
  scanf("%d", &i2);
  if (i1 >= 0 && i1 <= 10 && i2 >= 0 && i2 <= 10)
       if (regions[i1] && regions[i2]) {
          XorRegionRegion(regions[i1],regions[i2]);
          printf("Done.\n");
          return;
       }
  printf("Region with id %d or %d does not exist\n",i1,i2);
}

void andregionregion(void)
{
  int i1,i2;
  printf("Enter Region1 id: ");
  fflush(stdout);
  scanf("%d", &i1);
  printf("Enter Region2 (destination) id: ");
  fflush(stdout);
  scanf("%d", &i2);
  if (i1 >= 0 && i1 <= 10 && i2 >= 0 && i2 <= 10)
       if (regions[i1] && regions[i2]) {
          AndRegionRegion(regions[i1],regions[i2]);
          printf("Done.\n");
          return;
       }
  printf("Region with id %d or %d does not exist\n",i1,i2);
}
void orregionregion(void)
{
  int i1,i2;
  printf("Enter Region1 id: ");
  fflush(stdout);
  scanf("%d", &i1);
  printf("Enter Region2 (destination) id: ");
  fflush(stdout);
  scanf("%d", &i2);
  if (i1 >= 0 && i1 <= 10 && i2 >= 0 && i2 <= 10)
       if (regions[i1] && regions[i2]) {
          OrRegionRegion(regions[i1],regions[i2]);
          printf("Done.\n");
          return;
       }
  printf("Region with id %d or %d does not exist\n",i1,i2);
}

void doall(void)
{
    char buf[80];

    for (;;) {
	printf("> ");
	fflush(stdout);
	scanf("%s", buf);

	if (!strcmp(buf, "quit")) {
	    freeregions();
	    return;
	} else if (!strcmp(buf, "help")) {
	    printf("quit help newregion disposeregion clearregion\n");
	    printf("andrectregion orrectregion xorrectregion clearrectregion\n");
	    printf("andregionregion orregionregion xorregionregion\n");
	    printf("showregions showrects clear\n");
	} else if (!strcmp(buf, "newregion")) {
	    newregion();
	} else if (!strcmp(buf, "disposeregion")) {
	    disposeregion();
	} else if (!strcmp(buf, "clearregion")) {
	    clearregion();
	} else if (!strcmp(buf, "showregions")) {
	    showregions();
	} else if (!strcmp(buf, "showrects")) {
	    showrects();
	} else if (!strcmp(buf, "orrectregion")) {
	    orrectregion();
	} else if (!strcmp(buf, "andrectregion")) {
	    andrectregion();
	} else if (!strcmp(buf, "clearrectregion")) {
	    clearrectregion();
	} else if (!strcmp(buf, "xorrectregion")) {
	    xorrectregion();
	} else if (!strcmp(buf, "xorregionregion")) {
	    xorregionregion();
	} else if (!strcmp(buf, "orregionregion")) {
	    orregionregion();
	} else if (!strcmp(buf, "andregionregion")) {
	    andregionregion();
	} else if (!strcmp(buf, "clear")) {
	    SetRast(win->RPort, 0);
	} else
	    printf("Unknown command. Try 'help'.\n");
    }
}
