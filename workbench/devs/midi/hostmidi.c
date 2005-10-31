/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: debugdriver.c 12675 2001-11-10 23:23:05Z chodorowski $

    Desc: 
    Lang: English
*/



/*********************************************************************************
  Not a very good example at all. But at least it should prove that
  AROS is able to use camd-drivers. -Kjetil M.
  
  Compiling it up is easy. Its just like any other AROS-program, showed
  by this makefile:
  
"
debugdriver: debugdriver.o
    gcc -nostartfiles -nostdlib -Xlinker -i ../../lib/startup.o debugdriver.o -o debugdriver -L../../lib -larossupport -lamiga -larosc -larosm

debugdriver.o: debugdriver.c makefile
    gcc debugdriver.c -c -I../../Include -Wall
"

***********************************************************************************/


#include <exec/types.h>
#include <midi/camddevices.h>
#include <hidd/unixio.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <libcore/compiler.h>

#define NUMPORTS 1

struct ExecBase *SysBase;
struct Library  *OOPBase;

OOP_Object *unixio;
int 	    midi_fd;

int main(void)
{
    /* A camd mididriver is not supposed to be run directly, so we return an error. */

    return -1;
}



/*    Prototypes    */

extern void kprintf(char *bla,...);

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
    "HostMidi V41.0 (c) 2001 AROS - The Amiga Research OS",
    41,
    0,
    Init,
    Expunge,
    OpenPort,
    ClosePort,
    NUMPORTS,
    1
};

/****************************************************************
   We only store sysbase, thats all we need in this example.
   Otherwise, you may want to open libraries, set number of
   ports, obtain interrupts, etc.
 ***************************************************************/

SAVEDS ASM BOOL Init(REG(a6) APTR sysbase)
{
    SysBase=sysbase;

kprintf("hostmidi_init\n");
    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase) return FALSE;

    unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!unixio)
    {
        CloseLibrary(OOPBase);
        return FALSE;
    }

    midi_fd = Hidd_UnixIO_OpenFile((HIDD *)unixio, "/dev/midi",
    	    	    	    	   02, /* O_RDWR */
				   0, /* mode */
				   NULL); 

    if (!midi_fd)
    {
    	OOP_DisposeObject(unixio);
        CloseLibrary(OOPBase);
	
	return FALSE;
	
    }
    
    return TRUE;
}

/****************************************************************
   Nothing to do here. Normally, you may want to free memory,
   close some libraries, free some interrupts, etc.
*****************************************************************/
void Expunge(void)
{
    if (midi_fd) Hidd_UnixIO_CloseFile((HIDD *)unixio, midi_fd, NULL);
    if (unixio) OOP_DisposeObject(unixio);

    CloseLibrary(OOPBase);
}



ULONG (ASM *TransmitFunc)(REG(a2) APTR userdata);
APTR UserData[NUMPORTS];



/****************************************************************
   Normally, you may want to start an interrupt, or signal another task,
   or send a message to a port, that calls the transmit-function.
   But for this small example, sending the signal directly via
   kprintf is excactly what we want to do.
****************************************************************/

SAVEDS ASM void ActivateXmit(REG(a2) APTR userdata,ULONG REG(d0) portnum)
{
    ULONG data;
  
    for(;;)
    {
    	int written, errno;
	char buf[1];
	
    	data=(TransmitFunc)(userdata);

    	if(data==0x100) return;    

    	buf[0] = data;
	
        written = Hidd_UnixIO_WriteFile((HIDD *)unixio, midi_fd, buf, 1, &errno);    
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

