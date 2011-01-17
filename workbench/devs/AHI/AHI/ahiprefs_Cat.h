/****************************************************************
   This file was created automatically by `FlexCat 2.6.6'
   from "../../ahisrc/AHI/ahiprefs.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef ahiprefs_CAT_H
#define ahiprefs_CAT_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif


/*
**  Prototypes
*/
extern VOID OpenahiprefsCatalog(VOID);
extern VOID CloseahiprefsCatalog(VOID);

#ifdef LOCALIZE_V20
extern void InitahiprefsCatalog(STRPTR);
#endif



struct FC_String {
    const UBYTE *msg;
    LONG id;
};

extern struct FC_String ahiprefs_Strings[75];

#define msgMenuProject (ahiprefs_Strings[0].msg)
#define _msgMenuProject (ahiprefs_Strings+0)
#define msgItemOpen (ahiprefs_Strings[1].msg)
#define _msgItemOpen (ahiprefs_Strings+1)
#define msgItemSaveAs (ahiprefs_Strings[2].msg)
#define _msgItemSaveAs (ahiprefs_Strings+2)
#define msgItemAbout (ahiprefs_Strings[3].msg)
#define _msgItemAbout (ahiprefs_Strings+3)
#define msgItemQuit (ahiprefs_Strings[4].msg)
#define _msgItemQuit (ahiprefs_Strings+4)
#define msgMenuEdit (ahiprefs_Strings[5].msg)
#define _msgMenuEdit (ahiprefs_Strings+5)
#define msgItemDefaults (ahiprefs_Strings[6].msg)
#define _msgItemDefaults (ahiprefs_Strings+6)
#define msgItemLastSaved (ahiprefs_Strings[7].msg)
#define _msgItemLastSaved (ahiprefs_Strings+7)
#define msgItemRestore (ahiprefs_Strings[8].msg)
#define _msgItemRestore (ahiprefs_Strings+8)
#define msgMenuSettings (ahiprefs_Strings[9].msg)
#define _msgMenuSettings (ahiprefs_Strings+9)
#define msgItemCreateIcons (ahiprefs_Strings[10].msg)
#define _msgItemCreateIcons (ahiprefs_Strings+10)
#define msgMenuHelp (ahiprefs_Strings[11].msg)
#define _msgMenuHelp (ahiprefs_Strings+11)
#define msgItemHelp (ahiprefs_Strings[12].msg)
#define _msgItemHelp (ahiprefs_Strings+12)
#define msgItemUsersGuide (ahiprefs_Strings[13].msg)
#define _msgItemUsersGuide (ahiprefs_Strings+13)
#define msgItemConceptIndex (ahiprefs_Strings[14].msg)
#define _msgItemConceptIndex (ahiprefs_Strings+14)
#define msgPageMode (ahiprefs_Strings[15].msg)
#define _msgPageMode (ahiprefs_Strings+15)
#define msgPageAdvanced (ahiprefs_Strings[16].msg)
#define _msgPageAdvanced (ahiprefs_Strings+16)
#define msgUnitDevice (ahiprefs_Strings[17].msg)
#define _msgUnitDevice (ahiprefs_Strings+17)
#define msgUnitMusic (ahiprefs_Strings[18].msg)
#define _msgUnitMusic (ahiprefs_Strings+18)
#define msgProperties (ahiprefs_Strings[19].msg)
#define _msgProperties (ahiprefs_Strings+19)
#define msgPropRecordFull (ahiprefs_Strings[20].msg)
#define _msgPropRecordFull (ahiprefs_Strings+20)
#define msgPropRecordHalf (ahiprefs_Strings[21].msg)
#define _msgPropRecordHalf (ahiprefs_Strings+21)
#define msgPropRecordNone (ahiprefs_Strings[22].msg)
#define _msgPropRecordNone (ahiprefs_Strings+22)
#define msgOptions (ahiprefs_Strings[23].msg)
#define _msgOptions (ahiprefs_Strings+23)
#define msgOptFrequency (ahiprefs_Strings[24].msg)
#define _msgOptFrequency (ahiprefs_Strings+24)
#define msgOptChannels (ahiprefs_Strings[25].msg)
#define _msgOptChannels (ahiprefs_Strings+25)
#define msgOptNoChannels (ahiprefs_Strings[26].msg)
#define _msgOptNoChannels (ahiprefs_Strings+26)
#define msgOptVolume (ahiprefs_Strings[27].msg)
#define _msgOptVolume (ahiprefs_Strings+27)
#define msgOptMonitor (ahiprefs_Strings[28].msg)
#define _msgOptMonitor (ahiprefs_Strings+28)
#define msgOptGain (ahiprefs_Strings[29].msg)
#define _msgOptGain (ahiprefs_Strings+29)
#define msgOptMuted (ahiprefs_Strings[30].msg)
#define _msgOptMuted (ahiprefs_Strings+30)
#define msgOptInput (ahiprefs_Strings[31].msg)
#define _msgOptInput (ahiprefs_Strings+31)
#define msgOptNoInputs (ahiprefs_Strings[32].msg)
#define _msgOptNoInputs (ahiprefs_Strings+32)
#define msgOptOutput (ahiprefs_Strings[33].msg)
#define _msgOptOutput (ahiprefs_Strings+33)
#define msgOptNoOutputs (ahiprefs_Strings[34].msg)
#define _msgOptNoOutputs (ahiprefs_Strings+34)
#define msgGlobalOptions (ahiprefs_Strings[35].msg)
#define _msgGlobalOptions (ahiprefs_Strings+35)
#define msgGlobOptDebugLevel (ahiprefs_Strings[36].msg)
#define _msgGlobOptDebugLevel (ahiprefs_Strings+36)
#define msgGlobOptEcho (ahiprefs_Strings[37].msg)
#define _msgGlobOptEcho (ahiprefs_Strings+37)
#define msgGlobOptSurround (ahiprefs_Strings[38].msg)
#define _msgGlobOptSurround (ahiprefs_Strings+38)
#define msgGlobOptCPULimit (ahiprefs_Strings[39].msg)
#define _msgGlobOptCPULimit (ahiprefs_Strings+39)
#define msgDebugNone (ahiprefs_Strings[40].msg)
#define _msgDebugNone (ahiprefs_Strings+40)
#define msgDebugLow (ahiprefs_Strings[41].msg)
#define _msgDebugLow (ahiprefs_Strings+41)
#define msgDebugHigh (ahiprefs_Strings[42].msg)
#define _msgDebugHigh (ahiprefs_Strings+42)
#define msgDebugFull (ahiprefs_Strings[43].msg)
#define _msgDebugFull (ahiprefs_Strings+43)
#define msgEchoEnabled (ahiprefs_Strings[44].msg)
#define _msgEchoEnabled (ahiprefs_Strings+44)
#define msgEchoFast (ahiprefs_Strings[45].msg)
#define _msgEchoFast (ahiprefs_Strings+45)
#define msgEchoDisabled (ahiprefs_Strings[46].msg)
#define _msgEchoDisabled (ahiprefs_Strings+46)
#define msgSurroundEnabled (ahiprefs_Strings[47].msg)
#define _msgSurroundEnabled (ahiprefs_Strings+47)
#define msgSurroundDisabled (ahiprefs_Strings[48].msg)
#define _msgSurroundDisabled (ahiprefs_Strings+48)
#define msgButtonSave (ahiprefs_Strings[49].msg)
#define _msgButtonSave (ahiprefs_Strings+49)
#define msgButtonUse (ahiprefs_Strings[50].msg)
#define _msgButtonUse (ahiprefs_Strings+50)
#define msgButtonCancel (ahiprefs_Strings[51].msg)
#define _msgButtonCancel (ahiprefs_Strings+51)
#define msgButtonOK (ahiprefs_Strings[52].msg)
#define _msgButtonOK (ahiprefs_Strings+52)
#define msgTextProgramName (ahiprefs_Strings[53].msg)
#define _msgTextProgramName (ahiprefs_Strings+53)
#define msgTextCopyright (ahiprefs_Strings[54].msg)
#define _msgTextCopyright (ahiprefs_Strings+54)
#define msgTextNoWindow (ahiprefs_Strings[55].msg)
#define _msgTextNoWindow (ahiprefs_Strings+55)
#define msgTextNoFileRequester (ahiprefs_Strings[56].msg)
#define _msgTextNoFileRequester (ahiprefs_Strings+56)
#define msgTextNoFind (ahiprefs_Strings[57].msg)
#define _msgTextNoFind (ahiprefs_Strings+57)
#define msgTextNoOpen (ahiprefs_Strings[58].msg)
#define _msgTextNoOpen (ahiprefs_Strings+58)
#define msgGlobOptMasterVol (ahiprefs_Strings[59].msg)
#define _msgGlobOptMasterVol (ahiprefs_Strings+59)
#define msgMVNoClip (ahiprefs_Strings[60].msg)
#define _msgMVNoClip (ahiprefs_Strings+60)
#define msgMVClip (ahiprefs_Strings[61].msg)
#define _msgMVClip (ahiprefs_Strings+61)
#define msgGlobOptACTime (ahiprefs_Strings[62].msg)
#define _msgGlobOptACTime (ahiprefs_Strings+62)
#define msgButtonPlay (ahiprefs_Strings[63].msg)
#define _msgButtonPlay (ahiprefs_Strings+63)
#define msgOptScalemode (ahiprefs_Strings[64].msg)
#define _msgOptScalemode (ahiprefs_Strings+64)
#define msgSMFixedSafe (ahiprefs_Strings[65].msg)
#define _msgSMFixedSafe (ahiprefs_Strings+65)
#define msgSMDynSafe (ahiprefs_Strings[66].msg)
#define _msgSMDynSafe (ahiprefs_Strings+66)
#define msgSM0dB (ahiprefs_Strings[67].msg)
#define _msgSM0dB (ahiprefs_Strings+67)
#define msgSM3dB (ahiprefs_Strings[68].msg)
#define _msgSM3dB (ahiprefs_Strings+68)
#define msgSM6dB (ahiprefs_Strings[69].msg)
#define _msgSM6dB (ahiprefs_Strings+69)
#define msgFreqFmt (ahiprefs_Strings[70].msg)
#define _msgFreqFmt (ahiprefs_Strings+70)
#define msgChanFmt (ahiprefs_Strings[71].msg)
#define _msgChanFmt (ahiprefs_Strings+71)
#define msgVolFmt (ahiprefs_Strings[72].msg)
#define _msgVolFmt (ahiprefs_Strings+72)
#define msgACTimeFmt (ahiprefs_Strings[73].msg)
#define _msgACTimeFmt (ahiprefs_Strings+73)
#define msgPercentFmt (ahiprefs_Strings[74].msg)
#define _msgPercentFmt (ahiprefs_Strings+74)

#endif
