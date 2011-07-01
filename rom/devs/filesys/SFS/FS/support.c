#include "asmsupport.h"

#include <dos/dos.h>
#include <dos/bptr.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <strings.h>

#include "sysdep.h"
#include "blockstructure.h"
#include "fs.h"

#include "globals.h"
#include "debug.h"

void ClearMemQuick(void *mem, LONG bytes) {
  ULONG *m=mem;

  bytes>>=2;

  while(--bytes>=0) {
    *m++=0;
  }
}


#ifdef WORDCOMPRESSION

void uncompress(UWORD *dest,UBYTE *data,UWORD length) {
  UWORD *dataw;
  UBYTE *end=data+length;
  UBYTE code,len;

  /* Decompresses a transaction into /dest/ using /data/ which is
     /length/ bytes long. */

  while(data<end) {
    code=*data++;

    if((code & 0xC0)==0x40) {
      /* unchanged data */
      dest+=(code & 0x3F)+1;
    }
    else if((code & 0xC0)==0x80) {    // else if((code & 0xE0)==0x20) {
      /* set data */
      len=(code & 0x3F)+1;            // len=(code & 0x1F)+1;

      while(len--!=0) {
        *dest++=0xFFFF;
      }
    }
    else if((code & 0xC0)==0x00) {    // else if((code & 0xE0)==0x00) {
      /* clear data */
      len=(code & 0x3F)+1;            // len=(code & 0x1F)+1;

      while(len--!=0) {
        *dest++=0x0000;
      }
    }
    else {
      len=(code & 0x3F)+1;

      dataw=(UWORD *)data;
      while(len--!=0) {
        *dest++=*dataw++;
      }
      data=(UBYTE *)dataw;
    }
  }
}




WORD compress(UWORD *org,UWORD *new,UBYTE *dest) {
  WORD unchanged,cleared,set,immediate;
  WORD max;
  UWORD *o;
  UWORD *n;
  UWORD *end=new+(bytes_block>>1);
  UBYTE *begin=dest;

  while(new<end) {
    unchanged=0;
    cleared=0;
    set=0;

    max=end-new;
    if(max>64) {
      max=64;
    }

    o=org;
    n=new;

    while(*o++==*n++ && ++unchanged<max) {
    }

    if(unchanged<max) {
      n=new;

      while(*n++==0 && ++cleared<max) {
      }

      n=new;

      while(*n++==0xFFFF && ++set<max) {
      }
    }

    /* Largest of unchanged, cleared and set wins! */

    if(unchanged!=0 || cleared!=set) {
      UWORD x;

      if(unchanged>=cleared && unchanged>=set) {
        *dest++=0x40+unchanged-1;
        x=unchanged;
      }
      else if(cleared!=0) {
        *dest++=cleared-1;
        x=cleared;
      }
      else {
        *dest++=0x80+set-1;        // *dest++=0x20+set-1;
        x=set;
      }

      org+=x;
      new+=x;
    }
    else {
      /* Data wasn't unchanged, cleared or set */

      immediate=0;

      o=org;
      n=new;

      while(*n!=0 && *n!=0xFFFF && *o++!=*n++ && ++immediate<max) {
      }

      org+=immediate;

      *dest++=0xC0+immediate-1;
      o=(UWORD *)dest;               /* 68020 only! */
      while(immediate-->0) {
        *o++=*new++;
      }
      dest=(UBYTE *)o;
    }
  }

  return((WORD)(dest-begin));
}



WORD compressfromzero(UWORD *new,UBYTE *dest) {
  WORD cleared,set,immediate;
  WORD max;
  UWORD *n;
  UWORD *end=new+(bytes_block>>1);
  UBYTE *begin=dest;

  while(new<end) {
    cleared=0;
    set=0;

    max=end-new;
    if(max>64) {
      max=64;
    }

    n=new;

    while(*n++==0 && ++cleared<max) {
    }

    n=new;

    while(*n++==0xFFFF && ++set<max) {
    }

    /* Largest of cleared and set wins! */

    if(cleared!=set) {
      UWORD x;

      if(cleared!=0) {
        *dest++=cleared-1;
        x=cleared;
      }
      else {
        *dest++=0x80+set-1;        // *dest++=0x20+set-1;
        x=set;
      }

      new+=x;
    }
    else {
      /* Data wasn't cleared or set */

      immediate=0;

      n=new;

      while(*n!=0 && *n++!=0xFFFF && ++immediate<max) {
      }

      *dest++=0xC0+immediate-1;
      n=(UWORD *)dest;               /* 68020 only! */
      while(immediate-->0) {
        *n++=*new++;
      }
      dest=(UBYTE *)n;
    }
  }

  return((WORD)(dest-begin));
}

#endif


#ifdef LONGCOMPRESSION

void uncompress(ULONG *dest,UBYTE *data,UWORD length) {
  ULONG *dataw;
  UBYTE *end=data+length;
  UBYTE code,len;

  /* Decompresses a transaction into /dest/ using /data/ which is
     /length/ bytes long. */

  while(data<end) {
    code=*data++;

    if((code & 0xC0)==0x40) {
      /* unchanged data */
      dest+=(code & 0x3F)+1;
    }
    else if((code & 0xC0)==0x80) {    // else if((code & 0xE0)==0x20) {
      /* set data */
      len=(code & 0x3F)+1;            // len=(code & 0x1F)+1;

      while(len--!=0) {
        *dest++=0xFFFFFFFF;
      }
    }
    else if((code & 0xC0)==0x00) {    // else if((code & 0xE0)==0x00) {
      /* clear data */
      len=(code & 0x3F)+1;            // len=(code & 0x1F)+1;

      while(len--!=0) {
        *dest++=0x00000000;
      }
    }
    else {
      len=(code & 0x3F)+1;

      dataw=(ULONG *)data;
      while(len--!=0) {
        *dest++=*dataw++;
      }
      data=(UBYTE *)dataw;
    }
  }
}




WORD compress(ULONG *org,ULONG *new,UBYTE *dest) {
  WORD unchanged,cleared,set,immediate;
  WORD max;
  ULONG *o;
  ULONG *n;
  ULONG *end=new+(globals->bytes_block>>2);
  UBYTE *begin=dest;

  while(new<end) {
    unchanged=0;
    cleared=0;
    set=0;

    max=end-new;
    if(max>64) {
      max=64;
    }

    o=org;
    n=new;

    while(*o++==*n++ && ++unchanged<max) {
    }

    if(unchanged<max) {
      n=new;

      while(*n++==0 && ++cleared<max) {
      }

      n=new;

      while(*n++==0xFFFFFFFF && ++set<max) {
      }
    }

    /* Largest of unchanged, cleared and set wins! */

    if(unchanged!=0 || cleared!=set) {
      UWORD x;

      if(unchanged>=cleared && unchanged>=set) {
        *dest++=0x40+unchanged-1;
        x=unchanged;
      }
      else if(cleared!=0) {
        *dest++=cleared-1;
        x=cleared;
      }
      else {
        *dest++=0x80+set-1;        // *dest++=0x20+set-1;
        x=set;
      }

      org+=x;
      new+=x;
    }
    else {
      /* Data wasn't unchanged, cleared or set */

      immediate=0;

      o=org;
      n=new;

      while(*n!=0 && *n!=0xFFFFFFFF && *o++!=*n++ && ++immediate<max) {
      }

      org+=immediate;

      *dest++=0xC0+immediate-1;
      o=(ULONG *)dest;               /* 68020 only! */
      while(immediate-->0) {
        *o++=*new++;
      }
      dest=(UBYTE *)o;
    }
  }

  return((WORD)(dest-begin));
}



WORD compressfromzero(ULONG *new,UBYTE *dest) {
  WORD cleared,set,immediate;
  WORD max;
  ULONG *n;
  ULONG *end=new+(globals->bytes_block>>2);
  UBYTE *begin=dest;

  while(new<end) {
    cleared=0;
    set=0;

    max=end-new;
    if(max>64) {
      max=64;
    }

    n=new;

    while(*n++==0 && ++cleared<max) {
    }

    n=new;

    while(*n++==0xFFFFFFFF && ++set<max) {
    }

    /* Largest of cleared and set wins! */

    if(cleared!=set) {
      UWORD x;

      if(cleared!=0) {
        *dest++=cleared-1;
        x=cleared;
      }
      else {
        *dest++=0x80+set-1;        // *dest++=0x20+set-1;
        x=set;
      }

      new+=x;
    }
    else {
      /* Data wasn't cleared or set */

      immediate=0;

      n=new;

      while(*n!=0 && *n++!=0xFFFFFFFF && ++immediate<max) {
      }

      *dest++=0xC0+immediate-1;
      n=(ULONG *)dest;               /* 68020 only! */
      while(immediate-->0) {
        *n++=*new++;
      }
      dest=(UBYTE *)n;
    }
  }

  return((WORD)(dest-begin));
}

#endif



#ifdef NOCOMPRESSION

void uncompress(ULONG *dest,UBYTE *data,UWORD length) {
  CopyMem(data,dest,length);
}




WORD compress(ULONG *org,ULONG *new,UBYTE *dest) {
  CopyMem(new,dest,bytes_block);

  return((WORD)bytes_block);
}



WORD compressfromzero(ULONG *new,UBYTE *dest) {
  CopyMem(new,dest,bytes_block);

  return((WORD)bytes_block);
}

#endif


#ifdef ALTERNATIVECOMPRESSION

void uncompress(ULONG *dest, UBYTE *data, UWORD length) {
  ULONG *datal;
  ULONG *d;
  WORD l;
  UBYTE *dataend=data+length;

  if(*data++==1) {
    l=bytes_block>>4;
    d=dest;

    while(l-->0) {
      *d++=0;
      *d++=0;
      *d++=0;
      *d++=0;
    }
  }

  while(data<dataend) {
    d=dest + *data++;
    l=*data++;

    datal=(ULONG *)data;

    while(l-->0) {
      *d++=*datal++;
    }

    data=(UBYTE *)datal;
  }
}



UWORD compress(ULONG *org, ULONG *new, UBYTE *dest) {
  ULONG *destl;
  UWORD longs_block=bytes_block>>2;
  UWORD longsleft=longs_block;
  UBYTE count;
  UBYTE *deststart=dest;

  *dest++=0;

  for(;;) {
    while(*org==*new) {
      org++;
      new++;
      if(--longsleft==0) {
        return((UWORD)(dest-deststart));
      }
    }

    *dest++=longs_block-longsleft;
    destl=(ULONG *)(dest+1);
    count=0;

    do {
      count++;
      org++;
      *destl++=*new++;
      if(--longsleft==0) {
        *dest=count;
        dest=(UBYTE *)destl;
        return((UWORD)(dest-deststart));
      }
    } while(*org!=*new);

    *dest=count;
    dest=(UBYTE *)destl;
  }
}



UWORD compressfromzero(ULONG *new, UBYTE *dest) {
  ULONG *destl;
  UWORD longs_block=bytes_block>>2;
  UWORD longsleft=longs_block;
  UBYTE count;
  UBYTE *deststart=dest;

  *dest++=1;

  for(;;) {
    while(*new==0) {
      new++;
      if(--longsleft==0) {
        return((UWORD)(dest-deststart));
      }
    }

    *dest++=longs_block-longsleft;
    destl=(ULONG *)(dest+1);
    count=0;

    do {
      count++;
      *destl++=*new++;
      if(--longsleft==0) {
        *dest=count;
        dest=(UBYTE *)destl;
        return((UWORD)(dest-deststart));
      }
    } while(*new!=0);

    *dest=count;
    dest=(UBYTE *)destl;
  }
}

#endif



UBYTE *stripcolon(UBYTE *path) {
  UBYTE *path2;

  /* Finds the last colon in the path string (if any) and returns a pointer
     to the character following it.  If no colon is found you get a pointer
     to the first character in the string. */

  path2=path;
  while(*path2!=0) {
    if(*path2++==':') {
      path=path2;
    }
  }
  return(path);
}



UBYTE upperchar(UBYTE c) {
  if((c>=224 && c<=254 && c!=247) || (c>='a' && c<='z')) {
    c-=32;
  }
  return(c);
}



UWORD hash(UBYTE *name, WORD casesensitive) {
  UWORD hash=0;

  /* Calculates a hash value over the passed in string.  The end of the string
     can be either a NUL byte or a slash.  The hash function is the same as the
     one used in FastFileSystem set to international mode. */

  while(name[hash]!=0 && name[hash]!='/') {
    hash++;
  }

  if(casesensitive==FALSE) {
    while(*name!=0 && *name!='/') {
      hash=hash*13+upperchar(*name++);
    }
  }
  else {
    while(*name!=0 && *name!='/') {
      hash=hash*13+*name++;
    }
  }

  return(hash);
}


UBYTE *validatepath(UBYTE *string) {
  UBYTE *d;
  UBYTE *r;
  UBYTE c;
  WORD cnt;

  /* This functions limits the length of any path-part of the passed in string
     to max_name_length characters.  It also strips the colons. */

  string=stripcolon(string);

  d=string;
  r=string;

  cnt=globals->max_name_length;

  while((c=*string++)!=0) {
    if(c=='/') {
      cnt=globals->max_name_length+1;
    }
    if(--cnt>=0) {
      *d++=c;
    }
  }

  *d=0;
  
  return(r);
}



BYTE isvalidcomponentname(UBYTE *name) {
  UBYTE c;

  /* This function returns FALSE if the passed name
     is empty or contains a slash or colon. */

  if(name==0 || *name==0) {
    return(FALSE);
  }

  while((c=*name++)!=0) {
    if(c==':' || c=='/') {
      return(FALSE);
    }
  }

  return(TRUE);
}


void copystr(UBYTE *src,UBYTE *dest,WORD maxlen) {

  /* maxlen is the maximum stringlength the destination can become, excluding zero
     termination. */

  while(--maxlen>=0 && (*dest++=*src++)!=0) {
  }

  if(maxlen<0) {
    *dest=0;
  }
}



#ifdef USE_FAST_BSTR

UWORD copybstrasstr(BSTR bstr,UBYTE *str,UWORD maxlen)
{
  UBYTE *srcstr = BADDR(bstr);
  UWORD srclen;

  /* maxlen is the maximum stringlength the destination can become, excluding zero
     termination.  The return value is the length of the destination string also
     excluding termination. */

  srclen = strlen(srcstr);
  if(srclen<maxlen) {
    maxlen=srclen;
  }
  srclen=maxlen;

  while(maxlen--!=0) {
    *str++ = *srcstr++;
  }
  *str=0;

  return(srclen);
}

#else

UWORD bstrlen(UBYTE *str) {
  UWORD len;

  len=str[0];
  if(len!=0 && str[len]==0) {
    len--;
  }
  return(len);
}

UWORD copybstrasstr(BSTR bstr,UBYTE *str,UWORD maxlen) {
  UBYTE *srcstr=BADDR(bstr);
  UWORD srclen=bstrlen(srcstr);

  /* maxlen is the maximum stringlength the destination can become, excluding zero
     termination.  The return value is the length of the destination string also
     excluding termination. */

  srcstr++;
  if(srclen<maxlen) {
    maxlen=srclen;
  }
  srclen=maxlen;

  while(maxlen--!=0) {
    *str++=*srcstr++;
  }
  *str=0;

  return(srclen);
}

#endif


void initlist(struct List *list) {
  list->lh_Head=(struct Node *)&list->lh_Tail;
  list->lh_Tail=0;
  list->lh_TailPred=(struct Node *)list;
}




ULONG datestamptodate(struct DateStamp *datestamp) {
  return( (ULONG)((UWORD)datestamp->ds_Tick/50) + (ULONG)((UWORD)datestamp->ds_Minute*60) + datestamp->ds_Days*86400 );
}



void datetodatestamp(ULONG date,struct DateStamp *datestamp) {
  ULONG seconds;

  datestamp->ds_Days = date/86400;

  seconds=date - (datestamp->ds_Days * 86400);

  datestamp->ds_Minute = (ULONG)((UWORD)(seconds/60));
  datestamp->ds_Tick = (ULONG)((UWORD)(seconds%60)*50);
}



ULONG getdate(void) {
  struct DateStamp datestamp;

  DateStamp(&datestamp);

  return( datestamptodate(&datestamp) );
}


#if 0

void mergediffs(ULONG *current, UBYTE *olddiff, UWORD length, UBYTE *newdiff, UWORD *offsets) {
  ULONG *newdiffl;
  ULONG *c;

  *newdiff++=*olddiff++;

  bytestoskip=*offset & 0x0003;
  longoffset=(*offsets++)>>2;

  if(longoffset<*olddiff) {
    c=current+longoffset;

    *newdiff++=longoffset;
    newdiffl=(ULONG *)(newdiff+1);

    *newdiffl++=*c++;
    if(bytestoskip!=0) {
      *newdiffl++=*c++;
      *newdiff=2;
    }
    else
      *newdiff=1;
    }

    newdiff=(UBYTE *)newdiffl;
  }

  ...

}

void mergediffs(UBYTE *diff, UWORD length, UBYTE *dest, UWORD offsets[], UBYTE lengths[], UBYTE data[]) {
  UWORD unitsdone=0;
  WORD n;

  byteoffset=offsets[n];

  code=*diff++;
  len=(code & 0x3F)+1;

  if((unitsdone+len)<<2 > byteoffset) {

    /* Current code contains part of data which needs to be modified. */

    if((code & 0x40)==0) {         /* Clear or Set */

    }
  }
  else {
    *dest++=code;

    ...
  }
}



void adddiff(UBYTE *previouscode, UBYTE newcode, ) {

}



void mergediffs(UBYTE *diff, UWORD length, UBYTE *newdiff, UWORD newlength, UBYTE *dest) {
  UWORD newunitsdone=0;   /* Words/Longs already merged. */
  UWORD unitsdone=0;
  UWORD len;
  UBYTE code;

  /* Merges two diffs into a single diff. */

  /* 0x00 -> clear,  0x40 -> unchanged,  0x80 -> set,  0xC0 -> copy */


  code=*newdiff++;
  len=(code & 0x3F)+1;

  if((code & 0x40)==0) {         /* Clear or Set */
    *dest++=code;
    newunitsdone+=len;
  }
  else if((code & 0x80)!=0) {    /* Copy */
    ULONG *newdiffl;
    ULONG *destl;

    *dest++=code;
    newunitsdone+=len;

    destl=(ULONG *)dest;
    newdiffl=(ULONG *)newdiff;

    while(len--!=0) {
      *destl++=*newdiffl++;
    }

    dest=(UBYTE *)destl;
    newdiff=(UBYTE *)newdiffl;
  }
  else {   /* Unchanged */

    /* In this case the new diff contains a block of unchanged data.
       This means we need to check the old diff to see what needs to
       be done there. */

    code2=*diff++;
    len2=(code2 & 0x3F)+1;

    if(unitsdone+len2 > newunitsdone) {
      UWORD offset,size;

      /* Current code in old diff is overlapping the unchanged area. */

      offset=newunitsdone-unitsdone;
      size=unitsdone+len2 - newunitsdone;
      if(size>len) {  /* Check if overlapping area is smaller */
        size=len;
      }

      /* Calculated size and offset of overlapping area. */


    }
    else {
      if((code2 & 0xC0)==0xC0) {   /* Copy */
        /* Skips any data for copy. */
        diff+=len2<<2;
      }
      unitsdone+=len2;
    }

  }
}
















Transactions
------------

Operations are currently a diff of the original and the new
block.  Creating this diff takes a lot of time and can
reduce performance considerably.

It should be possible to merge 2 diffs quickly without
having to creating an entirely new diff.  This can be
accomplished by creating a function which takes 2 diffs and
merges them into one, letting the newer diff take
precedence.

Another function which would be needed is a function which
can create a diff based on the information to be changed
only.  Old code which changes just a single piece of
information in a CacheBuffer looks like this:


 preparecachebuffer(cb);

 o->object.file.size=x;

 errorcode=storecachebuffer(cb);


Instead it could be much faster if written like this:


 changecachebuffer(cb, &o->object.file.size, x);















/* Format:

1 byte offset, 1 byte length.  In LONGS.

*/

void uncompress(ULONG *dest, UBYTE *data, UWORD length) {
  ULONG *datal;
  ULONG *d;
  WORD l;
  UBYTE *dataend=data+length;

  if(*data++==1) {
    l=bytes_block>>4;
    d=dest;

    while(l-->0) {
      *d++=0;
      *d++=0;
      *d++=0;
      *d++=0;
    }
  }

  while(data<dataend) {
    d=dest + *data++;
    l=*data++;

    datal=(ULONG *)data;

    while(l-->0) {
      *d++=*datal++;
    }

    data=(UBYTE *)datal;
  }
}



UWORD compress(ULONG *org, ULONG *new, UBYTE *dest) {
  ULONG *destl;
  UWORD longs_block=bytes_block>>2;
  UWORD longsleft=longs_block;
  UBYTE count;
  UBYTE *deststart=dest;

  *dest++=0;

  for(;;) {
    while(*org==*new) {
      org++;
      new++;
      if(--longsleft==0) {
        return(dest-deststart);
      }
    }

    *dest++=longs_block-longsleft;
    destl=(ULONG *)(dest+2);
    count=0;

    do {
      count++;
      org++;
      *destl++=*new++;
      if(--longsleft==0) {
        *dest=count;
        dest=(UBYTE *)destl;
        return(dest-deststart);
      }
    } while(*org!=*new);

    *dest=count;
    dest=(UBYTE *)destl;
  }
}



UWORD compressfromzero(ULONG *new, UBYTE *dest) {
  ULONG *destl;
  UWORD longs_block=bytes_block>>2;
  UWORD longsleft=longs_block;
  UBYTE count;
  UBYTE *deststart=dest;

  *dest++=1;

  for(;;) {
    while(*new==0) {
      new++;
      if(--longsleft==0) {
        return(dest-deststart);
      }
    }

    *dest++=longs_block-longsleft;
    destl=(ULONG *)(dest+2);
    count=0;

    do {
      count++;
      *destl++=*new++;
      if(--longsleft==0) {
        *dest=count;
        dest=(UBYTE *)destl;
        return(dest-deststart);
      }
    } while(*new!=0);

    *dest=count;
    dest=(UBYTE *)destl;
  }
}

#endif





#ifdef BLOCKCOMPRESSION



/*
Diff compression scheme.

Depending on the block size there will be a special
header block which contains 2 bits for every 32 bytes
the block consists of.  We'll assume blocks of 512
bytes for now.
*/

UWORD makediff(ULONG *new, ULONG *org, ULONG **io_diff) {
  ULONG *diff=*io_diff;
  ULONG data;
  UWORD mode=0;
  WORD n;

  for(n=0; n<8; n++) {
    mode<<=2;
    data=*new++;

    if(data==0) {
      mode|=2;
    }
    else if(data==0xFFFFFFFF) {
      mode|=3;
    }
    else if(data!=*org) {
      mode|=1;
      *diff++=data;
    }

    org++;
  }

  *io_diff=diff;

  return(mode);
}



UWORD makedifffromzero(ULONG *new, ULONG **io_diff) {
  ULONG *diff=*io_diff;
  ULONG data;
  UWORD mode=0;
  WORD n;

  for(n=0; n<8; n++) {
    mode<<=2;
    data=*new++;

    if(data==0) {
      mode|=2;
    }
    else if(data==0xFFFFFFFF) {
      mode|=3;
    }
    else {
      mode|=1;
      *diff++=data;
    }
  }

  *io_diff=diff;

  return(mode);
}

#if 0

      if(newfilesize!=gh->size) {
        struct CacheBuffer *cb;
        struct fsObject *o;

        if((errorcode=readobject(lock->objectnode,&cb,&o))==0) {
          UBYTE modifiedblocks[4]={0,0,0,255};

          preparecachebuffer(cb);

          checksum_writelong(cb->data, &o->object.file.size, newfilesize);

          gh->size=newfilesize;

          modifiedblocks[1]=(&o->object.file.size - cb->data)>>5;
          modifiedblocks[2]=(&o->object.file.size - cb->data+3)>>5;

          errorcode=changecachebuffer(cb, modifiedblocks)
        }
      }

void checksum_writelong(struct fsBlockHeader *bh, void *dest, ULONG data) {
  ULONG original;
  /* Only handles longs written to even addresses! */
  original=*((ULONG *)dest);
  *((ULONG *)dest)=data;

  if(( ((UBYTE *)bh - (UBYTE *)dest) & 0x03)!=0) {
    /* Word aligned address. */

    original=(original<<16)|(original>>16);
    data=(data<<16)|(data>>16);
  }
  bh->checksum=~(~bh->checksum - original + data);
}

#endif




UWORD mergediffs(UBYTE *olddiff, UBYTE *newdiff, UWORD length, ULONG *new, ULONG *org, UBYTE *modifiedblocks) {
  ULONG *header=(ULONG *)newdiff;
  ULONG mc,newmc=0;
  UWORD *modesstart=(UWORD *)(newdiff+bytes_block+(bytes_block>>4));
  UWORD *modes=(UWORD *)(newdiff+bytes_block+(bytes_block>>4));
  UWORD *oldmodes=(UWORD *)(olddiff+length);
  UWORD *newdiffw;
  WORD b;

  mc=*((ULONG *)olddiff);
  olddiff+=4;

  for(b=0; b<16; b++) {
    newmc<<=2;

    if(b==*modifiedblocks) {
      UWORD mode;

      while(*++modifiedblocks==b) {
      }

      if(org==0) {
        mode=makedifffromzero(new, (ULONG **)&newdiff);
      }
      else {
        mode=makediff(new, org, (ULONG **)&newdiff);
      }

      if(mode==0x5555) {        // All copied.
        newmc|=1;
      }
      else if(mode==0xAAAA) {   // All cleared.
        newmc|=2;
      }
      else if(mode!=0) {
        newmc|=3;
        *--modes=mode;
      }
    }
    else {
      switch(mc & 0xC0000000) {
      case 0x40000000:
        {
          ULONG *newdiffl=(ULONG *)newdiff;
          ULONG *olddiffl=(ULONG *)olddiff;
          WORD n=8;

          newmc|=1;

          while(n-->0) {
            *newdiffl++=*olddiffl++;
          }

          newdiff=(UBYTE *)newdiffl;
          olddiff=(UBYTE *)olddiffl;
        }
        break;
      case 0x80000000:
        newmc|=2;
        break;
      case 0xC0000000:
        {
          ULONG *newdiffl=(ULONG *)newdiff;
          ULONG *olddiffl=(ULONG *)olddiff;
          UWORD mode;
          WORD n=8;

          newmc|=3;

          mode=*--oldmodes;
          *--modes=mode;

          while(n-->0) {
            if((mode & 0xC000)==0x4000) {
              *newdiffl++=*olddiffl++;
            }
            mode<<=2;
          }

          newdiff=(UBYTE *)newdiffl;
          olddiff=(UBYTE *)olddiffl;
        }
      }
    }

    new+=8;
    org+=8;
  }

  *header=newmc;

  newdiffw=(UWORD *)newdiff;

  while(modes!=modesstart) {
    *newdiffw++=*modes++;
  }

  return((UWORD)((UBYTE *)newdiffw-(UBYTE *)header));
}


void uncompress(ULONG *dest, UBYTE *diff, UWORD length) {
  ULONG mc;
  UWORD *modes=(UWORD *)(diff+length);
  WORD b=16;

  mc=*((ULONG *)diff);
  diff+=4;

  while(b-->0) {
    switch(mc & 0xC0000000) {
    case 0x00000000:
      dest+=8;
      break;
    case 0x40000000:
      {
        ULONG *diffl=(ULONG *)diff;
        WORD n=8;

        while(n-->0) {
          *dest++=*diffl++;
        }

        diff=(UBYTE *)diffl;
      }
      break;
    case 0x80000000:
      {
        WORD n=8;

        while(n-->0) {
          *dest++=0;
        }
      }
      break;
    default:
      {
        ULONG *diffl=(ULONG *)diff;
        UWORD mode=*--modes;
        WORD n=8;

        while(n-->0) {
          switch(mode & 0xC000) {
          case 0x0000:
            dest++;
            break;
          case 0x4000:
            *dest++=*diffl++;
            break;
          case 0x8000:
            *dest++=0x00000000;
            break;
          default:
            *dest++=0xFFFFFFFF;
          }

          mode<<=2;
        }

        diff=(UBYTE *)diffl;
      }
    }

    mc<<=2;
  }
}



  /* Blocks will be processed 32 bytes at the time.

  Master Control block:

  %00 = unchanged, no control word.
  %01 = 32 bytes of data, no control word.
  %10 = cleared (compress from zero!).
  %11 = use control word.

  Control WORD:

  %00 = unchanged (0x0000)
  %01 = use 32-bit value (0x5555)
  %10 = cleared (0xAAAA)
  %11 = set (0xFFFF) */



UWORD compress(ULONG *new, ULONG *org, UBYTE *diff) {
  ULONG *header=(ULONG *)diff;
  ULONG mc=0;
  UWORD *modesstart=(UWORD *)(diff+bytes_block+(bytes_block>>4));
  UWORD *modes=(UWORD *)(diff+bytes_block+(bytes_block>>4));
  UWORD *diffw;
  UWORD mode;
  WORD b=16;

  diff+=4;

  while(b-->0) {
    mc<<=2;

    mode=makediff(new, org, (ULONG **)&diff);

    if(mode==0x5555) {
      mc|=1;
    }
    else if(mode==0xAAAA) {
      mc|=2;
    }
    else if(mode!=0) {
      mc|=3;
      *--modes=mode;
    }

    new+=8;
    org+=8;
  }

  *header=mc;

  diffw=(UWORD *)diff;

  while(modes!=modesstart) {
    *diffw++=*modes++;
  }

  return((UWORD)((UBYTE *)diffw-(UBYTE *)header));
}



UWORD compressfromzero(ULONG *new, UBYTE *diff) {
  ULONG *header=(ULONG *)diff;
  ULONG mc=0;
  UWORD *modesstart=(UWORD *)(diff+bytes_block+(bytes_block>>4));
  UWORD *modes=(UWORD *)(diff+bytes_block+(bytes_block>>4));
  UWORD *diffw;
  UWORD mode;
  WORD b=16;

  diff+=4;

  while(b-->0) {
    mc<<=2;

    mode=makedifffromzero(new, (ULONG **)&diff);

    if(mode==0x5555) {
      mc|=1;
    }
    else if(mode==0xAAAA) {
      mc|=2;
    }
    else {
      mc|=3;
      *--modes=mode;
    }

    new+=8;
  }

  *header=mc;

  diffw=(UWORD *)diff;

  while(modes!=modesstart) {
    *diffw++=*modes++;
  }

  return((UWORD)((UBYTE *)diffw-(UBYTE *)header));
}



#endif




void checksum_writelong_be(struct fsBlockHeader *bh, void *dest, ULONG data) {
  ULONG original;

  /* Only handles longs written to even addresses! */

  original=BE2L(*((ULONG *)dest));
  *((ULONG *)dest)=L2BE(data);

  if(( ((UBYTE *)bh - (UBYTE *)dest) & 0x03)!=0) {

    /* Word aligned address. */

    original=(original<<16)|(original>>16);
    data=(data<<16)|(data>>16);
  }

  bh->be_checksum=~L2BE((~BE2L(bh->be_checksum) - original + data));
}

void checksum_writelong(struct fsBlockHeader *bh, void *dest, ULONG data) {
#if 0
  ULONG original;

  /* Only handles longs written to even addresses! */

  original=*((ULONG *)dest);
  *((ULONG *)dest)=data;

  if(( ((UBYTE *)bh - (UBYTE *)dest) & 0x03)!=0) {

    /* Word aligned address. */

    original=(original<<16)|(original>>16);
    data=(data<<16)|(data>>16);
  }

  bh->be_checksum=~(~bh->be_checksum - original + data);
#else
    checksum_writelong_be(bh, dest, data);
#endif
}
