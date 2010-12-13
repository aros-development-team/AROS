/*
  These SAS/C autoopen/autoclose functions was supplied by Mattias
  Karlsson. Thanks!
*/

#define USE_AHI_VERSION 4

#include <devices/ahi.h>
#include <proto/ahi.h>
#include <proto/exec.h>
#include <dos.h>

extern void __regargs __autoopenfail(char *);

extern long __oslibversion;

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

// Autoopen rutin för ahi.
int __stdargs _STI_openahi(void)
{

	if(AHImp=CreateMsgPort())
  	{
		if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest)))
		{
			AHIio->ahir_Version=USE_AHI_VERSION;

			if(!(AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL)))
		    {
				AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;
				return 0;
			}
		}
	}

	// Så vi får rätt version angivelse
	__oslibversion=USE_AHI_VERSION;
	__autoopenfail("ahi.device");
     return 1;
}

// autoclose rutin finns det oxo :)
void __stdargs _STD_closeahi(void)
{

	if(AHIDevice==0)
	{
        CloseDevice((struct IORequest *)AHIio);
	}

	if(AHIio)
	{
      DeleteIORequest((struct IORequest *)AHIio);
      AHIio=NULL;
    }

	if(AHImp)
	{
    	DeleteMsgPort(AHImp);
	    AHImp=NULL;
	}
}
