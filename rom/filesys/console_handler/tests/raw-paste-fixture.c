#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <datatypes/textclass.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
struct Library *IFFParseBase;
int main(int argc,char **argv){
 size_t n=argc>1?strtoul(argv[1],0,10):8192;if(n>16000)return 2;
 unsigned char *data=malloc(n?n:1);if(!data)return 3;
 for(size_t i=0;i<n;i++)data[i]=(i%79==78)?'\n':('a'+i%26);
 if(n>132)memcpy(data+128,"\x9b" "0 v",4);
 FILE *f=fopen("RAM:paste-expected.bin","wb");if(!f)return 4;
 if(fwrite(data,1,n,f)!=n||fclose(f))return 5;
 IFFParseBase=OpenLibrary("iffparse.library",0);if(!IFFParseBase)return 6;
 struct IFFHandle *iff=AllocIFF();if(!iff)return 7;
 iff->iff_Stream=(IPTR)OpenClipboard(0);if(!iff->iff_Stream)return 8;
 InitIFFasClip(iff);if(OpenIFF(iff,IFFF_WRITE))return 9;
 if(PushChunk(iff,ID_FTXT,ID_FORM,IFFSIZE_UNKNOWN))return 10;
 if(PushChunk(iff,0,ID_CHRS,IFFSIZE_UNKNOWN))return 11;
 LONG wrote=WriteChunkBytes(iff,data,n);PopChunk(iff);PopChunk(iff);CloseIFF(iff);
 CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);FreeIFF(iff);CloseLibrary(IFFParseBase);free(data);
 if(wrote!=(LONG)n)return 12;printf("CLIP_FIXTURE_READY %lu\n",(unsigned long)n);return 0;
}
