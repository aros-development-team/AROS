#ifndef _GAUGE_PRIVATE_H_
#define _GAUGE_PRIVATE_H_

#define GAUGE_BUFSIZE 256

/*** Instance data **********************************************************/
struct Gauge_DATA
{
   BOOL horiz;
   BOOL dupinfo;

   ULONG current; /* (dlc) LONG in MUI, (sba) but MUI seems to handle it like ULONG */
   ULONG max;
   ULONG divide;
   STRPTR info;

   char buf[GAUGE_BUFSIZE];
   LONG info_width;
   LONG info_height;
};


#endif /* _GAUGE_PRIVATE_H_ */
