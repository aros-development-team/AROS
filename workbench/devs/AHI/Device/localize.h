
/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "/cygdrive/e/Private/Projects/AROS-Win32/contrib/necessary/AHI/Device/ahi.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef ahi_CAT_H
#define ahi_CAT_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif	/*  !EXEC_TYPES_H	    */
#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif	/*  !LIBRARIES_LOCALE_H     */


/*  Prototypes	*/
extern void OpenahiCatalog(struct Locale *, STRPTR);
extern void CloseahiCatalog(void);
extern STRPTR GetahiString(APTR);

extern struct Catalog * ExtOpenCatalog(struct Locale *, STRPTR);
extern void ExtCloseCatalog(struct Catalog *);
extern STRPTR GetString(APTR, struct Catalog *);


/*  Definitions */
extern const APTR _msgDefault;
#define msgDefault ((APTR) &_msgDefault)
extern const APTR _msgMenuControl;
#define msgMenuControl ((APTR) &_msgMenuControl)
extern const APTR _msgMenuLastMode;
#define msgMenuLastMode ((APTR) &_msgMenuLastMode)
extern const APTR _msgMenuNextMode;
#define msgMenuNextMode ((APTR) &_msgMenuNextMode)
extern const APTR _msgMenuPropertyList;
#define msgMenuPropertyList ((APTR) &_msgMenuPropertyList)
extern const APTR _msgMenuRestore;
#define msgMenuRestore ((APTR) &_msgMenuRestore)
extern const APTR _msgMenuOK;
#define msgMenuOK ((APTR) &_msgMenuOK)
extern const APTR _msgMenuCancel;
#define msgMenuCancel ((APTR) &_msgMenuCancel)
extern const APTR _msgUnknown;
#define msgUnknown ((APTR) &_msgUnknown)
extern const APTR _msgReqOK;
#define msgReqOK ((APTR) &_msgReqOK)
extern const APTR _msgReqCancel;
#define msgReqCancel ((APTR) &_msgReqCancel)
extern const APTR _msgReqFrequency;
#define msgReqFrequency ((APTR) &_msgReqFrequency)
extern const APTR _msgDefaultMode;
#define msgDefaultMode ((APTR) &_msgDefaultMode)
extern const APTR _msgReqInfoTitle;
#define msgReqInfoTitle ((APTR) &_msgReqInfoTitle)
extern const APTR _msgReqInfoAudioID;
#define msgReqInfoAudioID ((APTR) &_msgReqInfoAudioID)
extern const APTR _msgReqInfoResolution;
#define msgReqInfoResolution ((APTR) &_msgReqInfoResolution)
extern const APTR _msgReqInfoMono;
#define msgReqInfoMono ((APTR) &_msgReqInfoMono)
extern const APTR _msgReqInfoStereo;
#define msgReqInfoStereo ((APTR) &_msgReqInfoStereo)
extern const APTR _msgReqInfoStereoPan;
#define msgReqInfoStereoPan ((APTR) &_msgReqInfoStereoPan)
extern const APTR _msgReqInfoChannels;
#define msgReqInfoChannels ((APTR) &_msgReqInfoChannels)
extern const APTR _msgReqInfoMixrate;
#define msgReqInfoMixrate ((APTR) &_msgReqInfoMixrate)
extern const APTR _msgReqInfoHiFi;
#define msgReqInfoHiFi ((APTR) &_msgReqInfoHiFi)
extern const APTR _msgReqInfoRecordHalf;
#define msgReqInfoRecordHalf ((APTR) &_msgReqInfoRecordHalf)
extern const APTR _msgReqInfoRecordFull;
#define msgReqInfoRecordFull ((APTR) &_msgReqInfoRecordFull)
extern const APTR _msgReqInfoMultiChannel;
#define msgReqInfoMultiChannel ((APTR) &_msgReqInfoMultiChannel)
extern const APTR _msgFreqFmt;
#define msgFreqFmt ((APTR) &_msgFreqFmt)

#endif /*   !ahi_CAT_H  */
