/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Speedtest of function and DoMethod() call
    Lang: english
*/

#define INTUI_V36_NAMES_ONLY

#include <stdio.h>

#include <aros/config.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gfxhidd.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>

#include <proto/alib.h>
#include <proto/intuition.h>


struct Library        *GfxHiddBase;
struct IntuitionBase *IntuitionBase;
struct DosLibrary    *DOSBase;

struct Object *obj;
struct DateStamp date1, date2;
struct hGfx_SpeeTest msg;
ULONG  ticks;
ULONG  i, cnt = 500000;


ULONG DiffStamp(struct DateStamp date1, struct DateStamp date2)
{
    ULONG ticks = 0;

    ticks = date2.ds_Tick - date1.ds_Tick;
    ticks = ticks + 50 * 60 * (date2.ds_Minute - date1.ds_Minute);

    return(ticks);
}


int main(int argc, char **argv)
{
  msg.MethodID = HIDDM_SpeedTest;
  msg.val1 = 1;
  msg.val2 = 2;
  msg.val3 = 3;

  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 37);
  if(IntuitionBase)
  {

      DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 37);
      if(DOSBase)
      {

          GfxHiddBase = (struct Library *) OpenLibrary("aros:libs/gfxhidd.library", 0);
          if(GfxHiddBase)
          {
              printf("Libs open\n");
    
              obj = NewObject(NULL, GRAPHICSHIDD, TAG_END);
              if(obj)
              {
                  /* Function */
                  printf("Test: HIDDF_Graphics_TestSpeed(1, 2, 3\n");
                  DateStamp(&date1);
                  for(i = 0; i < cnt; i++)
                      HIDDF_Graphics_TestSpeed(1, 2, 3);

                  DateStamp(&date2);
                  ticks = DiffStamp(date1, date2);
                  printf("Seconds: %li Ticks: %li\n\n", ticks / 50, ticks % 50);


                  /* DoMethod */
                  printf("Test: DoMethod((ULONG *)obj, HIDDM_SpeedTest, 1, 2, 3)\n");
                  DateStamp(&date1);
                  for(i = 0; i < cnt; i++)
                      DoMethod((ULONG *)obj, HIDDM_SpeedTest, 1, 2, 3);

                  DateStamp(&date2);
                  ticks = DiffStamp(date1, date2);
                  printf("Seconds: %li Ticks: %li\n\n", ticks / 50, ticks % 50);


                  /* DoMethodA */
                  printf("Test: DoMethodA((ULONG *)obj, (Msg) &msg)\n");
                  DateStamp(&date1);
                  for(i = 0; i < cnt; i++)
                      DoMethodA((ULONG *)obj, (Msg) &msg);

                  DateStamp(&date2);
                  ticks = DiffStamp(date1, date2);
                  printf("Seconds: %li Ticks: %li\n\n", ticks / 50, ticks % 50);

                  DisposeObject(obj);
              }

              CloseLibrary((struct Library *)GfxHiddBase);
          }

          CloseLibrary((struct Library *)DOSBase);
      }

      CloseLibrary((struct Library *)IntuitionBase);
  }
}

