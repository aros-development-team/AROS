/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_AHI_SUB_H
#define _INLINE_AHI_SUB_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef AHI_SUB_BASE_NAME
#define AHI_SUB_BASE_NAME AHIsubBase
#endif /* !AHI_SUB_BASE_NAME */

#define AHIsub_AllocAudio(___tagList, ___AudioCtrl) \
	AROS_LC2(ULONG, AHIsub_AllocAudio, \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 5, Ahi_sub)

#define AHIsub_FreeAudio(___AudioCtrl) \
	AROS_LC1(void, AHIsub_FreeAudio, \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 6, Ahi_sub)

#define AHIsub_Disable(___AudioCtrl) \
	AROS_LC1(void, AHIsub_Disable, \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 7, Ahi_sub)

#define AHIsub_Enable(___AudioCtrl) \
	AROS_LC1(void, AHIsub_Enable, \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 8, Ahi_sub)

#define AHIsub_Start(___Flags, ___AudioCtrl) \
	AROS_LC2(ULONG, AHIsub_Start, \
	AROS_LCA(ULONG, (___Flags), D0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 9, Ahi_sub)

#define AHIsub_Update(___Flags, ___AudioCtrl) \
	AROS_LC2(ULONG, AHIsub_Update, \
	AROS_LCA(ULONG, (___Flags), D0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 10, Ahi_sub)

#define AHIsub_Stop(___Flags, ___AudioCtrl) \
	AROS_LC2(ULONG, AHIsub_Stop, \
	AROS_LCA(ULONG, (___Flags), D0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 11, Ahi_sub)

#define AHIsub_SetVol(___Channel, ___Volume, ___Pan, ___AudioCtrl, ___Flags) \
	AROS_LC5(ULONG, AHIsub_SetVol, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(Fixed, (___Volume), D1), \
	AROS_LCA(sposition, (___Pan), D2), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D3), \
	struct Library *, AHI_SUB_BASE_NAME, 12, Ahi_sub)

#define AHIsub_SetFreq(___Channel, ___Freq, ___AudioCtrl, ___Flags) \
	AROS_LC4(ULONG, AHIsub_SetFreq, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(ULONG, (___Freq), D1), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D2), \
	struct Library *, AHI_SUB_BASE_NAME, 13, Ahi_sub)

#define AHIsub_SetSound(___Channel, ___Sound, ___Offset, ___Length, ___AudioCtrl, ___Flags) \
	AROS_LC6(ULONG, AHIsub_SetSound, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(UWORD, (___Sound), D1), \
	AROS_LCA(ULONG, (___Offset), D2), \
	AROS_LCA(LONG, (___Length), D3), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D4), \
	struct Library *, AHI_SUB_BASE_NAME, 14, Ahi_sub)

#define AHIsub_SetEffect(___Effect, ___AudioCtrl) \
	AROS_LC2(ULONG, AHIsub_SetEffect, \
	AROS_LCA(APTR, (___Effect), A0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 15, Ahi_sub)

#define AHIsub_LoadSound(___Sound, ___Type, ___Info, ___AudioCtrl) \
	AROS_LC4(ULONG, AHIsub_LoadSound, \
	AROS_LCA(UWORD, (___Sound), D0), \
	AROS_LCA(ULONG, (___Type), D1), \
	AROS_LCA(APTR, (___Info), A0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 16, Ahi_sub)

#define AHIsub_UnloadSound(___Sound, ___Audioctrl) \
	AROS_LC2(ULONG, AHIsub_UnloadSound, \
	AROS_LCA(UWORD, (___Sound), D0), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___Audioctrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 17, Ahi_sub)

#define AHIsub_GetAttr(___Attribute, ___Argument, ___DefValue, ___tagList, ___AudioCtrl) \
	AROS_LC5(LONG, AHIsub_GetAttr, \
	AROS_LCA(ULONG, (___Attribute), D0), \
	AROS_LCA(LONG, (___Argument), D1), \
	AROS_LCA(LONG, (___DefValue), D2), \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 18, Ahi_sub)

#define AHIsub_HardwareControl(___Attribute, ___Argument, ___AudioCtrl) \
	AROS_LC3(LONG, AHIsub_HardwareControl, \
	AROS_LCA(ULONG, (___Attribute), D0), \
	AROS_LCA(LONG, (___Argument), D1), \
	AROS_LCA(struct AHIAudioCtrlDrv *, (___AudioCtrl), A2), \
	struct Library *, AHI_SUB_BASE_NAME, 19, Ahi_sub)

#endif /* !_INLINE_AHI_SUB_H */
