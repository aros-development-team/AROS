*************************************************
*						*
*		 (C) Copyright 1996		*
*						*
*		   Tomi Blinnikka		*
*						*
*************************************************
*						*
* DISTRIBUTION:					*
*						*
* The complete and unmodified archive, as 	*
* provided by Tomi Blinnikka, may be freely 	*
* redistributed via any means which preserves 	*
* its copyrights. The archive may not be sold 	*
* for profit.					*
*						*
* Permission granted to be distributed with the	*
* AHI user/development software.		*
*						*
*************************************************
*						*
* DISCLAIMER:					*
*						*
* GETAUDIOID IS PROVIDED "AS-IS" AND SUBJECT TO *
* CHANGE; NO WARRANTIES ARE MADE.  ALL USE IS	*
* AT YOUR OWN RISK.  NO LIABILITY OR		*
* RESPONSIBILITY IS ASSUMED.			*
*						*
*************************************************
*						*
* DESCRIPTION:					*
*						*
* A program to display AHI AudioID requester	*
* and print selected ID in hex and decimal.	*
*						*
* Requires Kickstart 2.04+, ahi.device 1+.	*
*						*
*************************************************
*						*
* HISTORY:					*
*						*
* Version 0.01	29.04.1996			*
*	 -0.99ö 29.04.1996			*
*						*
* Basics.					*
*						*
*						*
* Version 1.00	30.04.1996			*
*						*
* Works from CLI/Shell & Workbench. 		*
*						*
* Added TITLE, POSITIVE, NEGATIVE, LFORMAT &	*
* PUBSCREEN options.				*
*						*
* Added Locale support.				*
*						*
*						*
* Version 1.01	01.05.1996			*
*						*
* Doesn't set an initial AudioID anymore,	*
* since Martin tells my shouldn't make		*
* assumptions about direct IDs.			*
*						*
* Recompiled with tags names from AHI beta 2.	*
*						*
* If no mode was selected then returns -5.	*
*						*
*						*
* BUGS:						*
*						*
*						*
*************************************************

	INCLUDE	"dos/dosextens.i"
	INCLUDE	"devices/ahi.i"
	INCLUDE "libraries/locale.i"
	INCLUDE "workbench/startup.i"

	INCLUDE "JMPLibs.i"		;Macros to open/close/call libs
	INCLUDE	"lvo/ahi_lib.i"		;ahi.device offsets

;XREFs needed

	INCLUDE "XREF:2.0.xref"
	INCLUDE "XREF:exec.xref"
	INCLUDE "XREF:dos.xref"
	INCLUDE "XREF:icon.xref"


;DEBUG_MODE	SET	1

AHI_DEFMIXFREQ:	EQU	30058		;Sounds a lot better than 11kHz...

PROGVERSION:	macro
		dc.b	"1.01 (01.05.96)"
		endm


;Use these and leave a few hundred include lines out

TRUE:		EQU	1
FALSE:		EQU	0

do_ToolTypes:	EQU	$36


		Section GAI,CODE

START:		push	all			;Save registers

		sub.l	a1,a1			;Find our task
		jlib	Exec,FindTask
		move.l	d0,a4
		move.l	pr_CLI(a4),d0
		bne	OpenDos

		lea.l	pr_MsgPort(a4),a0
		jlib	Exec,WaitPort
		lea.l	pr_MsgPort(a4),a0
		flib	Exec,GetMsg		;Get WB message
		move.l	d0,WBMsg

OpenDos:	openlib Dos,NoDos		;Keep at beginning

		moveq.l	#0,d1			;Go to root
		jlib	Dos,CurrentDir
		move.l	d0,OrigLock		;This is where we came from


;Open locale.library

		openlib	Locale,NoLocale

		sub.l	a0,a0
		jlib	Locale,OpenLocale
		move.l	d0,Locale1

		sub.l	a0,a0			;No locale, use default
		lea.l	CatalogName1,a1
		lea.l	CatalogTags,a2
		jlib	Locale,OpenCatalogA	;Open our catalog for this language
		move.l	d0,GAICatalog
		beq	NoLocale


;Here we set the text strings using locale support

		moveq.l	#0,d7
		lea.l	CatStrings,a3		;Def. String in A1

LocaleLoop:	move.l	d7,d0			;StringNum in D0
		move.l	GAICatalog,a0		;Catalog in A0
		move.l	(a3),a1			;Default string in A1
		jlib	Locale,GetCatalogStr

		move.l	d0,(a3)

		add.l	#1,d7
		add.l	#4,a3
		cmp.l	#CatStrAmount,d7
		bne	LocaleLoop


;Fix strings that aren't move.l'd

NoLocale:	move.l	STRAHIReqTitle1,OptTitle
		move.l	STRPositiveText2,OptPositive
		move.l	STRNegativeText2,OptNegative


		tst.l	WBMsg
		beq	CLIStart		;Skip WB stuff if from CLI


;Open CON: window where we print errors and the result.

		move.l	#CONName,d1
		move.l	#MODE_OLDFILE,d2
		jlib	Dos,Open
		move.l	d0,CONFile
		beq	ShutDown
		move.l	CONFile,_stdout

		openlib Icon,NoIcon

		move.l	WBMsg,a0
		move.l	sm_ArgList(a0),a1
		move.l	wa_Lock(a1),d1
		jlib	Dos,CurrentDir		;Change to the directory where our icon is


;The icon we open here can't be free'd until all the data from them has
;been dealt with, i.e. until the progrma ends.

		move.l	WBMsg,a0
		move.l	sm_ArgList(a0),a1
		move.l	wa_Name(a1),a0
		jlib	Icon,GetDiskObject	;Get icon
		move.l	d0,DiskObject
		beq	MainStart

DoToolType1:	move.l	d0,a4
		move.l	do_ToolTypes(a4),a0
		lea.l	DECIMALText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType2
		move.l	#1,OptDecimal		;This user wants decimal ID

DoToolType2:	move.l	do_ToolTypes(a4),a0
		lea.l	TITLEText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType3
		move.l	d0,OptTitle		;This title, please

DoToolType3:	move.l	do_ToolTypes(a4),a0
		lea.l	POSITIVEText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType4
		move.l	d0,OptPositive		;This positive gad text

DoToolType4:	move.l	do_ToolTypes(a4),a0
		lea.l	NEGATIVEText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType5
		move.l	d0,OptNegative		;This negative gad text

DoToolType5:	move.l	do_ToolTypes(a4),a0
		lea.l	LFORMATText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType6
		move.l	d0,OptLFormat		;Format with this

DoToolType6:	move.l	do_ToolTypes(a4),a0
		lea.l	PUBSCREENText1,a1
		flib	Icon,FindToolType
		tst.l	d0
		beq	DoToolType7
		move.l	d0,OptPubScreen		;Open on this public screen
DoToolType7:
		bra	MainStart		;Skip CLI stuff


CLIStart:	jlib	Dos,Output
		move.l	d0,_stdout

		move.l	#CLTemplate1,d1
		move.l	#CLArray1,d2
		moveq.l	#0,d3
		flib	Dos,ReadArgs		;Read options from CLI
		move.l	d0,RDArgs1
		beq	NoRDArgs


;Open ahi.device

MainStart:	OPENAHI	1
		move.l	d0,_AHIBase
		beq	NoAHI			;Open failed


;Let's set all the options correctly

		move.l	(OptTitle),ReqTitle
		move.l	(OptPositive),ReqPos
		move.l	(OptNegative),ReqNeg
		move.l	(OptPubScreen),ReqPubScr


;Here we go and check that the user specified formatting string doesn't
;contain more than two '%ld' strings.

		move.l	OptLFormat,d2
		beq	OpenAHIReq		;Check if available

		bsr	CheckLegality		;Check if valid
		tst.l	d2
		beq	BadFmt			;ERROR: Invalid


;Allocate AHI's audio requester

OpenAHIReq:	lea.l	AHIReqTags1,a0
		jlib	AHI,AHI_AllocAudioRequestA
		move.l	d0,AHIReq1
		beq	NoAHIReq		;Allocation failed


;Show requester

		move.l	d0,a0
		lea.l	AHIReqTags2,a1
		flib	AHI,AHI_AudioRequestA
		cmp.l	#TRUE,d0
		bne	CheckCancel		;Positive gad _not_ pressed

;Get values from requester and set then into the formatting template

		move.l	AHIReq1,a0

		cmp.l	#AHI_INVALID_ID,ahiam_AudioID(a0)
		beq	CheckCancel		;None selected

		lea.l	VFPrintfArgv,a1
		move.l	ahiam_AudioID(a0),0(a1)
		move.l	ahiam_MixFreq(a0),4(a1)

		move.l	#FmtString1,d2		;Default formatting string

		tst.l	OptDecimal		;Test for user's FmtString
		beq	CLIOpts2
		move.l	#FmtString2,d2		;Print audio ID in decimal

CLIOpts2:	tst.l	OptLFormat
		beq	PrintResult
		move.l	OptLFormat,d2		;Print audio ID in hex

PrintResult:	move.l	_stdout,d1
		move.l	a1,d3
		jlib	Dos,VFPrintf		;Print chosen mode

		tst.l	OptLFormat
		beq	ShutDown
		lea.l	CRLFText1,a0
		bsr	Printer			;The user string needs a CRLF pair

		bra	ShutDown


;ShutDown routine. This cleans up EVERYTHING.
;
;We have three ways to start it:
;
;1. ERRShutDown		- does a quick turn in Printer to print anything in a0 first
;2. ERRShutDown2	- makes sure we return an error code when finished
;3. ShutDown		- a normal or non-fatal shutdown


ERRShutDown:	bsr	Printer
ERRShutDown2:	move.l	#RETURN_FAIL,RetCode

ShutDown:	tst.l	_AHIBase
		beq	ShutDown7000
		move.l	AHIReq1,a0			;Allowed to be NULL
		jlib	AHI,AHI_FreeAudioRequest

ShutDown7000:	tst.l	CONFile
		beq	ShutDown6000
		move.l	CONFile,d1
		jlib	Dos,Close

ShutDown6000:	move.l	RDArgs1,d1
		beq	ShutDown5000
		jlib	Dos,FreeArgs

ShutDown5000:	tst.l	_DosBase
		beq	ShutDown4000
		move.l	OrigLock,d1			;Can be zero for root
		jlib	Dos,CurrentDir

ShutDown4000:	tst.l	DiskObject
		beq	ShutDown1500
		move.l	DiskObject,a0
		jlib	Icon,FreeDiskObject

ShutDown1500:	tst.l	_LocaleBase
		beq	ShutDown1000
		move.l	GAICatalog,a0
		jlib	Locale,CloseCatalog

ShutDown1000:	CLOSEAHI
		closlib	Locale
		closlib	Icon
		closlib	Dos

		move.l	WBMsg,d2
		beq	ShutDown100
		jlib	Exec,Forbid
		move.l	d2,a1
		flib	Exec,ReplyMsg

ShutDown100:	pull	all
		move.l	RetCode,d0
		rts


;Subroutines


;CheckLegality does a simple check on the user provided formatting string
;so that we hopefully can't cause any Enforcer hits or any other nasty
;things
;
;INPUTS	D2 = Pointer to formatting string
;
;OUTPUT	D2 = NULL for error or original string

CheckLegality:	move.l	d2,a0
		clr.b	d1			;'%' counter

CheckLegality2:	tst.b	(a0)
		beq	CheckLeg_OUT		;Do until NULL
		cmp.b	#'%',(a0)+
		bne	CheckLegality2
		cmp.b	#'%',(a0)		;Let user print %-sign
		bne	CheckLegality3
		add.l	#1,a0
		bra	CheckLegality2
CheckLegality3:	add.b	#1,d1
		cmp.b	#3,d1
		bne	CheckLegality2		;OK if 2 or under
CheckLegality4:	clr.l	d2			;Illegal
CheckLeg_OUT:	rts


;Error handling routines

NoDos:		pull	all
		move.l	#RETURN_FAIL,d0
		rts

CheckCancel:	move.l	#RETURN_WARN,RetCode	;Set return code to WARN
NoRDArgs:	jlib	Dos,IoErr
		move.l	d0,d1
		beq	ShutDown		;User cancelled
		moveq.l	#0,d2
		flib	Dos,PrintFault
		bra	ERRShutDown2

NoAHI:		move.l	STRNoAHIText1,a0
		bra	ERRShutDown

NoIcon:		move.l	STRNoIconText1,a0
		bra	ERRShutDown

NoAHIReq:	move.l	STRNoAHIReqText1,a0
		bra	ERRShutDown

BadFmt:		move.l	STRBadFormatText1,a0
		bra	ERRShutDown


;Normal print function with Enforcer hit checks

Printer:	cmp.l	#0,a0
		beq	Printer_OUT		;No string
		tst.b	(a0)
		beq	Printer_OUT		;No content
		move.l	_stdout,d1
		move.l	a0,d2
		jlib	Dos,FPuts		;Print buffered
Printer_OUT:	rts


;Reservations start here

		libnames			;This contains macros for the library names

		ds.l	0


;CLI/Shell stuff

RDArgs1:	dc.l	0			;CLI arguments
RetCode:	dc.l	RETURN_OK		;Return this on exit


;Options

CLArray1:
OptDecimal:	dc.l	0			;Print ID in decimal
OptTitle:	dc.l	0			;Window title
OptPositive:	dc.l	0			;Positive gad text 'OK'
OptNegative:	dc.l	0			;Negative gad text 'Cancel'
OptLFormat:	dc.l	0			;User defined formatting
OptPubScreen:	dc.l	NULLText1		;Screen to open on


;File stuff

CONFile:	dc.l	0			;Actuall a CON window
_stdout:	dc.l	0			;CLI or 'CON:'
OrigLock:	dc.l	0			;Directory where we started


;Workbench stuff

WBMsg:		dc.l	0			;WB message
DiskObject:	dc.l	0			;Icon


;AHI stuff

_AHIBase:	dc.l	0			;AHI device base
AHIReq1:	dc.l	0			;AudioID requester

VFPrintfArgv:	dc.l	0,0			;AHIAudioID, AHIMixFreq


;Locale stuff

GAICatalog:	dc.l	0			;Our translation file
Locale1:	dc.l	0			;Locale we're using


;Text strings

CLTemplate1:	dc.b	"DEC=DECIMAL/S,TITLE/K,POSITIVE/K,NEGATIVE/K,"
		dc.b	"LFORMAT/K,PUBSCREEN/K",0
DECIMALText1:	dc.b	"DECIMAL",0
TITLEText1:	dc.b	"TITLE",0
POSITIVEText1:	dc.b	"POSITIVE",0
NEGATIVEText1:	dc.b	"NEGATIVE",0
LFORMATText1:	dc.b	"LFORMAT",0
PUBSCREENText1:	dc.b	"PUBSCREEN",0
CONName:	dc.b	"CON:0/10/500/70/GetAudioID/CLOSE/AUTO/WAIT",0
Version:	dc.b	"$VER: GetAudioID "
		PROGVERSION
		dc.b	" (c) Copyright Tomi Blinnikka 1996",0
FmtString1:	dc.b	"AUDIOID=0x%08lx MIXFREQ=%ld\r\n",0
FmtString2:	dc.b	"AUDIOID=%ld MIXFREQ=%ld\r\n",0
CRLFText1:	dc.b	"\r\n"		;Continues on next line!
NULLText1:	dc.b	"",0


;Locale strings

EnglishText1:	dc.b	"english",0
CatalogName1:	dc.b	"GetAudioID.catalog",0


;Taglists

		ds.l	0		;Long align

;Tags for requester allocation

AHIReqTags1:	dc.l	AHIR_PubScreenName
ReqPubScr:	dc.l	0
		dc.l	TAG_DONE


;Tags for showing requester

AHIReqTags2:	dc.l	AHIR_TitleText
ReqTitle:	dc.l	0
		dc.l	AHIR_PositiveText
ReqPos:		dc.l	0
		dc.l	AHIR_NegativeText
ReqNeg:		dc.l	0
		dc.l	AHIR_DoMixFreq,TRUE			;Have slider for MixFrequency
		dc.l	AHIR_InitialMixFreq,AHI_DEFMIXFREQ	;Start with this MixFreq
		dc.l	TAG_DONE


;Locale tags

CatalogTags:	dc.l	OC_Language,0
		dc.l	OC_BuiltInLanguage,EnglishText1	;We have this language built-in
		dc.l	TAG_DONE


;Locale text strings

		INCDIR	"LWF:AHI/GetAudioID/"		;Change to your path
		INCLUDE	"GAI_locale.i"

		END
