#include <exec/libraries.h>
#include <libraries/codesets.h>
#include <proto/codesets.h>
#include <proto/exec.h>

#include <stdio.h>

/* This is just a very quickly written test, not a full-featured convertor */
#define BUF_SIZE 102400

struct Library *CodesetsBase;
struct codeset *srcCodeset;
struct codeset *destCodeset;

int main(int argc, char **argv)
{
  char *buf, *destbuf;
  ULONG destlen;
  FILE *f;

  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <source codeset> <destination codeset> <source file>\n", argv[0]);
    return 0;
  }
  CodesetsBase = OpenLibrary("codesets.library", 0);
  if (!CodesetsBase)
  {
    fprintf(stderr, "Failed to open codesets.library!\n");
    return 0;
  }
  srcCodeset = CodesetsFind(argv[1], CSA_FallbackToDefault, FALSE, TAG_DONE);
  if (srcCodeset)
  {
    destCodeset = CodesetsFind(argv[2], CSA_FallbackToDefault, FALSE, TAG_DONE);
    if (destCodeset)
    {
      buf = AllocMem(BUF_SIZE, MEMF_CLEAR);

      if (buf)
      {
	f = fopen(argv[3], "r");
	if (f)
	{
	  fread(buf, BUF_SIZE-1, 1, f);
	  fclose(f);
	  destbuf = CodesetsConvertStr(CSA_SourceCodeset, srcCodeset, CSA_DestCodeset, destCodeset,
				       CSA_Source, buf, CSA_DestLenPtr, &destlen, TAG_DONE);
	  if (destbuf)
	  {
	    fprintf(stderr, "Result length: %lu\n", destlen);
	    fwrite(destbuf, destlen, 1, stdout);
	    fputc('\n', stderr);
	    CodesetsFreeA(destbuf, NULL);
	  }
	  else
	    fprintf(stderr, "Failed to convert text!\n");
	}
	FreeMem(buf, BUF_SIZE);
      }
      else
	fprintf(stderr, "Failed to allocate %lu bytes for buffer\n", BUF_SIZE);
    }
    else
      fprintf(stderr, "Unknown destination codeset %s\n", argv[2]);
  }
  else
    fprintf(stderr, "Unknown source codeset %s\n", argv[1]);
  CloseLibrary(CodesetsBase);
  return 0;
}

