/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_AHI_H
#define _INLINE_AHI_H

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

#ifndef AHI_BASE_NAME
#define AHI_BASE_NAME AHIBase
#endif /* !AHI_BASE_NAME */

#define AHI_AllocAudioA(___tagList) \
	AROS_LC1(struct AHIAudioCtrl *, AHI_AllocAudioA, \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 7, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_AllocAudio(___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_AllocAudioA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_FreeAudio(___AudioCtrl) \
	AROS_LC1(void, AHI_FreeAudio, \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	struct Library *, AHI_BASE_NAME, 8, Ahi)

#define AHI_ControlAudioA(___AudioCtrl, ___tagList) \
	AROS_LC2(ULONG, AHI_ControlAudioA, \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 10, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_ControlAudio(___AudioCtrl, ___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_ControlAudioA((___AudioCtrl), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_SetVol(___Channel, ___Volume, ___Pan, ___AudioCtrl, ___Flags) \
	AROS_LC5(void, AHI_SetVol, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(Fixed, (___Volume), D1), \
	AROS_LCA(sposition, (___Pan), D2), \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D3), \
	struct Library *, AHI_BASE_NAME, 11, Ahi)

#define AHI_SetFreq(___Channel, ___Freq, ___AudioCtrl, ___Flags) \
	AROS_LC4(void, AHI_SetFreq, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(ULONG, (___Freq), D1), \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D2), \
	struct Library *, AHI_BASE_NAME, 12, Ahi)

#define AHI_SetSound(___Channel, ___Sound, ___Offset, ___Length, ___AudioCtrl, ___Flags) \
	AROS_LC6(void, AHI_SetSound, \
	AROS_LCA(UWORD, (___Channel), D0), \
	AROS_LCA(UWORD, (___Sound), D1), \
	AROS_LCA(ULONG, (___Offset), D2), \
	AROS_LCA(LONG, (___Length), D3), \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	AROS_LCA(ULONG, (___Flags), D4), \
	struct Library *, AHI_BASE_NAME, 13, Ahi)

#define AHI_SetEffect(___Effect, ___AudioCtrl) \
	AROS_LC2(ULONG, AHI_SetEffect, \
	AROS_LCA(APTR, (___Effect), A0), \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	struct Library *, AHI_BASE_NAME, 14, Ahi)

#define AHI_LoadSound(___Sound, ___Type, ___Info, ___AudioCtrl) \
	AROS_LC4(ULONG, AHI_LoadSound, \
	AROS_LCA(UWORD, (___Sound), D0), \
	AROS_LCA(ULONG, (___Type), D1), \
	AROS_LCA(APTR, (___Info), A0), \
	AROS_LCA(struct AHIAudioCtrl *, (___AudioCtrl), A2), \
	struct Library *, AHI_BASE_NAME, 15, Ahi)

#define AHI_UnloadSound(___Sound, ___Audioctrl) \
	AROS_LC2(void, AHI_UnloadSound, \
	AROS_LCA(UWORD, (___Sound), D0), \
	AROS_LCA(struct AHIAudioCtrl *, (___Audioctrl), A2), \
	struct Library *, AHI_BASE_NAME, 16, Ahi)

#define AHI_NextAudioID(___Last_ID) \
	AROS_LC1(ULONG, AHI_NextAudioID, \
	AROS_LCA(ULONG, (___Last_ID), D0), \
	struct Library *, AHI_BASE_NAME, 17, Ahi)

#define AHI_GetAudioAttrsA(___ID, ___Audioctrl, ___tagList) \
	AROS_LC3(BOOL, AHI_GetAudioAttrsA, \
	AROS_LCA(ULONG, (___ID), D0), \
	AROS_LCA(struct AHIAudioCtrl *, (___Audioctrl), A2), \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 18, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_GetAudioAttrs(___ID, ___Audioctrl, ___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_GetAudioAttrsA((___ID), (___Audioctrl), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_BestAudioIDA(___tagList) \
	AROS_LC1(ULONG, AHI_BestAudioIDA, \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 19, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_BestAudioID(___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_BestAudioIDA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_AllocAudioRequestA(___tagList) \
	AROS_LC1(struct AHIAudioModeRequester *, AHI_AllocAudioRequestA, \
	AROS_LCA(struct TagItem *, (___tagList), A0), \
	struct Library *, AHI_BASE_NAME, 20, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_AllocAudioRequest(___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_AllocAudioRequestA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_AudioRequestA(___Requester, ___tagList) \
	AROS_LC2(BOOL, AHI_AudioRequestA, \
	AROS_LCA(struct AHIAudioModeRequester *, (___Requester), A0), \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 21, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_AudioRequest(___Requester, ___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_AudioRequestA((___Requester), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_FreeAudioRequest(___Requester) \
	AROS_LC1(void, AHI_FreeAudioRequest, \
	AROS_LCA(struct AHIAudioModeRequester *, (___Requester), A0), \
	struct Library *, AHI_BASE_NAME, 22, Ahi)

#define AHI_PlayA(___Audioctrl, ___tagList) \
	AROS_LC2(void, AHI_PlayA, \
	AROS_LCA(struct AHIAudioCtrl *, (___Audioctrl), A2), \
	AROS_LCA(struct TagItem *, (___tagList), A1), \
	struct Library *, AHI_BASE_NAME, 23, Ahi)

#ifndef NO_INLINE_STDARG
#define AHI_Play(___Audioctrl, ___tag1, ...) \
	({_sfdc_vararg _tags[] = { ___tag1, __VA_ARGS__ }; AHI_PlayA((___Audioctrl), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define AHI_SampleFrameSize(___SampleType) \
	AROS_LC1(ULONG, AHI_SampleFrameSize, \
	AROS_LCA(ULONG, (___SampleType), D0), \
	struct Library *, AHI_BASE_NAME, 24, Ahi)

#define AHI_AddAudioMode(___AHIPrivate) \
	AROS_LC1(ULONG, AHI_AddAudioMode, \
	AROS_LCA(struct TagItem *, (___AHIPrivate), A0), \
	struct Library *, AHI_BASE_NAME, 25, Ahi)

#define AHI_RemoveAudioMode(___AHIPrivate) \
	AROS_LC1(ULONG, AHI_RemoveAudioMode, \
	AROS_LCA(ULONG, (___AHIPrivate), D0), \
	struct Library *, AHI_BASE_NAME, 26, Ahi)

#define AHI_LoadModeFile(___AHIPrivate) \
	AROS_LC1(ULONG, AHI_LoadModeFile, \
	AROS_LCA(STRPTR, (___AHIPrivate), A0), \
	struct Library *, AHI_BASE_NAME, 27, Ahi)

#endif /* !_INLINE_AHI_H */
