/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <midi/camddevices.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>

#define NUMPORTS 1

struct LibCInterface
{
    int	 (*open)(char *path, int oflag, ...);
    int	 (*close)(int filedes);
    long (*write)(int fildes, const void *buf, long nbyte);
};

struct ExecBase *SysBase;
APTR HostLibBase;
APTR LibcHandle = NULL;
struct LibCInterface *LibcIFace = NULL;
int midi_fd;

int __startup Main(void)
{
    /* A camd mididriver is not supposed to be run directly, so we return an error. */
    return -1;
}

/*    Prototypes    */

BOOL ASM Init(REG(a6) APTR sysbase);
void Expunge(void);
SAVEDS ASM struct MidiPortData *OpenPort(
					 REG(a3) struct MidiDeviceData *data,
					 REG(d0) LONG portnum,
					 REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
					 REG(a1) void (* ASM recievefunc)(UWORD REG(d0) input,APTR REG(a2) userdata),
					 REG(a2) APTR userdata
					 );
ASM void ClosePort(
		   REG(a3) struct MidiDeviceData *data,
		   REG(d0) LONG portnum
		   );

/*   End prototypes  */

/***********************************************************************
   The mididevicedata struct.
   Note. It doesn't have to be declared with the const qualifier, since
   NPorts may be set at Init. You should set the name-field to the
   same as the filename, that might be a demand...
***********************************************************************/
const struct MidiDeviceData mididevicedata =
{
    MDD_Magic,
    "hostmidi",
    "HostMidi V41.0 (c) 2001 AROS - The AROS Research OS",
    41,
    0,
    Init,
    Expunge,
    OpenPort,
    ClosePort,
    NUMPORTS,
    1
};

static const char *libc_symbols[] = {
    "open",
    "close",
    "write",
    NULL
};

SAVEDS ASM BOOL Init(REG(a6) APTR sysbase)
{
    ULONG r = 0;

    SysBase=sysbase;

    D(kprintf("hostmidi_init\n"));

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    	return FALSE;
    	
    LibcHandle = HostLib_Open("libc.so.6", NULL);
    if (!LibcHandle)
    	LibcHandle = HostLib_Open("c", NULL);

    if (!LibcHandle)
    	return FALSE;

    LibcIFace = (struct LibCInterface *)HostLib_GetInterface(LibcHandle, libc_symbols, &r); 
    if ((!LibcIFace) || r)
    {
    	HostLib_Close(LibcHandle, NULL);
    	return FALSE;
    }

    midi_fd = LibcIFace->open("/dev/midi", 02, 0); /* O_RDWR */

    if (midi_fd == -1)
    {
    	HostLib_DropInterface((APTR *)LibcIFace);
    	HostLib_Close(LibcHandle, NULL);

	return FALSE;
    }
    
    return TRUE;
}

void Expunge(void)
{
    if (!HostLibBase)
	return;

    if (LibcIFace)
    {
    	if (midi_fd != -1)
    	    LibcIFace->close(midi_fd);
    	HostLib_DropInterface((APTR *)LibcIFace);
    }

    if (LibcHandle)
    	HostLib_Close(LibcHandle, NULL);
}

ULONG (ASM *TransmitFunc)(REG(a2) APTR userdata);
APTR UserData[NUMPORTS];

SAVEDS ASM void ActivateXmit(REG(a2) APTR userdata,ULONG REG(d0) portnum)
{
    ULONG data;
  
    for(;;)
    {
	char buf[1];
	
    	data=(TransmitFunc)(userdata);

    	if(data==0x100) return;    

    	buf[0] = data;
	LibcIFace->write(midi_fd, buf, 1);
    }
}

struct MidiPortData midiportdata=
{
    ActivateXmit
};


/****************************************************************
   This one is called whenever a program that has opened
   camd.library wants to use your services.
****************************************************************/
SAVEDS ASM struct MidiPortData *OpenPort(
					 REG(a3) struct MidiDeviceData *data,
					 REG(d0) LONG portnum,
					 REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
					 REG(a1) void (* ASM recieverfunc)(UWORD REG(d0) input,APTR REG(a2) userdata),
					 REG(a2) APTR userdata
					 )
{
    /* We haven't got any receiver function, so we don't bother about storing the receiverfunc variable. */

    TransmitFunc=transmitfunc;
    UserData[portnum-1]=userdata;
    
    return &midiportdata;
}



/****************************************************************
   Nothing to do here. Normally, you may want to free memory,
   mark the port not to be in use anymore, delete a task, etc.
*****************************************************************/
ASM void ClosePort(
		   REG(a3) struct MidiDeviceData *data,
		   REG(d0) LONG portnum
		   )
{
    return;
}

