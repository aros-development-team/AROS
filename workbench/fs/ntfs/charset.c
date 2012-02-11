#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ntfs_fs.h"
#include "charset.h"

static ULONG readLine(BPTR fh, char *buf, ULONG size)
{
  char *c;

  if((c = FGets(fh, buf, size)) == NULL)
    return FALSE;

  for(; *c; c++)
  {
    if(*c == '\n' || *c == '\r')
    {
      *c = '\0';
      break;
    }
  }

  return TRUE;
}

void InitCharsetTables(void)
{
	int i;

	for (i = 0; i < 65536; i++)
	if (i < 256) {
		glob->from_unicode[i] = i;
		glob->to_unicode[i] = i;
	} else
		glob->from_unicode[i] = '_';
}

// Reads a coding table
BOOL ReadUnicodeTable(STRPTR name)
{
  BPTR fh;

  fh = Open(name, MODE_OLDFILE);
  if (fh)
  {
      int i, n;
      char buf[512];

      while(readLine(fh, buf, 512*sizeof(char)))
      {
	if(!isdigit(*buf))
          continue;
        else
        {
          char *p = buf;
          int fmt2 = 0;

          if((*p=='=') || (fmt2 = ((*p=='0') || (*(p+1)=='x'))))
          {
            p++;
            p += fmt2;

            i = strtol((const char *)p,(char **)&p,16);
	    if(i>=0 && i<256)
            {
              while(isspace(*p)) p++;

              if(!strnicmp(p, "U+", 2))
              {
                p += 2;
		n = strtol((const char *)p,(char **)&p,16);
              }
              else
              {
		if(*p!='#')
		  n = strtol((const char *)p,(char **)&p,0);
		else
		  n = -1;
              }
	      if (n >= 0 && n < 65536) {
		glob->from_unicode[n] = i;
		glob->to_unicode[i] = n;
	      }
            }
          }
        }
      }
      Close(fh);
  }
  return fh ? TRUE : FALSE;
}

