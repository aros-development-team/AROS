	IFND	DAD_AUDIO_I
DAD_AUDIO_H	SET	1

* C to ASM conversion done by Martin Blom (lcs@lysator.liu.se) 1995-12-16.

*************************************************************************
** $VER: 0.19_BETA dad_audio.h (95.11.15)                              **
** dad_audio.device include file BETA by Johan Nyblom                  **
** email: nyblom@mother.ludd.luth.se                                   **
** All this knowledge has been found by reverse engineering,           **
** guessing and the use of devmon. It has NOTHING to do with           **
** Digital Audio Design, the makers of wavetools hardware              **
** and software.                                                       **
** I am not responsible for any damage anyone does with this stuff :)  **
** just so that you know, this is EXTREMELY shaky knowledge.           **
** Ye have been forewarned!                                            **
*************************************************************************

	include exec/io.i	* std DEVICE commands and structures *

*  Standard device commands, listed for easy access.
* CMD_INVALID	0 Invalid Command
* CMD_RESET	1 Reset device to orig.state
* CMD_READ	2 Read from device
* CMD_WRITE	3 Write to device
* CMD_UPDATE	4 Process buffer
* CMD_CLEAR	5 Clear all buffers
* CMD_STOP	6 Insert pause
* CMD_START	7 Continue after pause
* CMD_FLUSH	8 Stop current task
*

*************************************************************************
**                                                                     **
** dad_audio.device	   					       **
**                                                                     **
*************************************************************************

** CMD_READ	standard device command
**		io_Data = Longword Aligned.
**			data is read into the address in io_Data.
**		io_Offset = -1, it worls with Offset=0 but dad_audio.device
**			returns offset=-1 so why not use it from the start,
**			besides WaveTools(software) uses Offset -1 so...
**
**		Result: buffer is read to address,
**			io_Actual = bytes read
**
**
** DataFormat is LONG( WORD(left) , WORD(right))
**
**



**
** dad_audio.device commandon
**

DADCMD_BUFFER		EQU	(CMD_NONSTD+0)
DAD_BUFFER_SETUP	EQU	0
DAD_BUFFER_SWITCH	EQU	-1

	** Call with Offset to determine usage,
	** Offset = 	DAD_ BUFFER_SWITCH
	**		Result: io_Actual = number of bytes in presampled buffer.
	**
	** Offset = 	DAD_BUFFER_SETUP
	** 		Result:	io_Actual is set to first sampling length (ie max internal buffer size)
	**		usually 8084 but it could differ with another hardware setup.
	**
	**Model:
	**	Suppose the card has 2 onboard buffes, these buffers are 8084 bytes each.
	** 1.	Setup onboard buffers, returning maximum size in bytes. (8084)
	** 2.	Next read from the card to a buffer MEMF_24BITDMA or
	**		MEMF_CHIP if you are gonna dump it do disk,
	**		the card starts sampling into buffer1 and let me read it when it is full,
	**		while the card continues to sample into buffer2.
	** 3.	I am done reading and now switches buffers with the buffer command,
	**		buffer2 is frozen. and sampling to buffer 1 starts,
	**		io_Actual is the length the card managed to sample into buffer2.
	**		io_Error = -128 if overflow(or some error) happened.
	** 4.	I read from card, getting my data from buffer2 this time.
	**	Then it starts over at 3.
        **



DADCMD_OUTPUTDAMP	EQU	(CMD_NONSTD+2)
DADCONST_MAXDAMP	EQU	31		* value from dad_audio.device *
	**
	** Set damping on output, io_Data = Damping
	** The maximum is 31 which gives no sound out and minimum is 0, integer steps.
	** all other except flags and command should be zero
        **


DADCMD_INPUTGAIN	EQU	(CMD_NONSTD+3)
DADCONST_MAXGAIN	EQU	15
	**
	** Set gain on input, io_Data = gain
	** Maximum = 15, minimum = 0 integer steps
	**


DADCMD_REPLAYFREQ	EQU	(CMD_NONSTD+4)
	**
	** set playback frequency io_Data = Frequency.
	** You can only use a couple of frequencies
	** see below DADFREQ_*
	**


DADCMD_INIT1		EQU	(CMD_NONSTD+5)
	**
	** This is called by WaveTools (sampling software)
	** with all values = 0 except command,
	** upon return io_Data = an address which is later used
	** in the set buffer command. I dont know what this is about
	** maybe it is auto buffer allocation. Maybe it is base address of the
	** hardware dma zone or something. But since you can set your own bufferspace,
	** it doesnt matter.
	**


DADCMD_MUTE		EQU	(CMD_NONSTD+6)
	**
	** Mute Internal channels output
	** io_Data = 0 turns sound off
	** io_Data = 1 turns them on
	**


DADCMD_SAMPLEFREQ	EQU	(CMD_NONSTD+7)
	**
	** set sampling frequency io_Data = frequency
	** I checked dad_audio.device and it did no real checking of the
	** frequency, maybe it is linked to replay frequency.
	**


DADCMD_SMPTE		EQU	(CMD_NONSTD+15)
	**
	** SMPTE port init, E=-1 if hardware is not there.
	** I cant check anything more because I havent got the hardware :(
	**


DADCMD_INIT2		EQU	(CMD_NONSTD+17)
	**
	** This is an interresting command, it is called with
	** io_Data = (DADF_SET | DADF_INIT) (ie. $80000001) by wavetools
	** prior to setting frequency,damping and such.
	** I dont know what it is, could be internal status bits or something.
	**


DAD_DEVICENAME MACRO
	dc.b	"dad_audio.device",0
	ENDM

**
** Flags
**
DADB_INIT	EQU	0
DADB_SETFLAG	EQU	31

DADF_INIT	EQU	(1<<0)	* $00000001 *
DADF_SETFLAG	EQU	(1<<31)	* $80000000 *


**
** Frequencies
**
DADFREQ_48000	EQU	48000	* These values are checked for in dad_audio.device 	*
DADFREQ_44100	EQU	44100	* I guess they are the only frequencys alowed		*
DADFREQ_32000	EQU	32000	* they are valid for both sampling and playback		*
DADFREQ_29400	EQU	29400
DADFREQ_24000	EQU	24000
DADFREQ_22050	EQU	22050
DADFREQ_19200	EQU	19200
DADFREQ_17640	EQU	17640


	ENDC * DAD_AUDIO_H  *
