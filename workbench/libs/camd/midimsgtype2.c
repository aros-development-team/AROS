/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/


#include "camd_intern.h"

WORD MidiMsgType_status(UBYTE status){
	switch(status&0xf0){
		case 0xf0:
			if(status>=0xf8) return CMB_RealTime;
			if(status==0xf0) return CMB_SysEx;
			return CMB_SysCom;
		case 0x80:
		case 0x90:
			return CMB_Note;
		case 0xa0:
			return CMB_PolyPress;
		case 0xb0:
			return 0xff;
		case 0xc0:
			return CMB_Prog;
		case 0xd0:
			return CMB_ChanPress;
		case 0xe0:
			return CMB_PitchBend;
	}
	return 0xff;	// Never reached. (hopefully)
}

WORD MidiMsgType_CMB_Ctrl(UBYTE data1){
	if(data1<32) return CMB_CtrlMSB;
	if(data1<64) return CMB_CtrlLSB;
	if(data1<80) return CMB_CtrlSwitch;
	if(data1<96) return CMB_CtrlByte;
	if(data1<102) return CMB_CtrlParam;
	if(data1<120) return CMB_CtrlUndef;
	return CMB_Mode;
}

WORD MidiMsgType_status_data1(UBYTE status,UBYTE data1){
	WORD ret=MidiMsgType_status(status);
	if(ret==0xff){
		return MidiMsgType_CMB_Ctrl(data1);
	}
	return ret;
}


