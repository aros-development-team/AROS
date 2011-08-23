/* Automatically generated gatestubs! Do not edit! */

#include <dos/dos.h>
#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/ahi_sub.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __NOLIBBASE__
#include <proto/ahi_sub.h>
#undef __NOLIBBASE__
#include <stdarg.h>

struct Library*
_LibInit(struct Library* ___library, BPTR ___seglist, struct ExecBase* ___SysBase);

struct Library*
gwLibInit(	struct Library* ___library,
	BPTR ___seglist,
	struct ExecIFace* _iface)
{
  struct ExecBase* ___SysBase = (struct ExecBase*) _iface->Data.LibBase;
  return _LibInit(___library, ___seglist, ___SysBase);
}

struct Library*
_LibOpen(struct Library * _base);

struct Library*
gwLibOpen(struct LibraryManagerInterface* _iface)
{
  return _LibOpen((struct Library *) _iface->Data.LibBase);
}

BPTR
_LibClose(struct Library * _base);

BPTR
gwLibClose(struct LibraryManagerInterface* _iface)
{
  return _LibClose((struct Library *) _iface->Data.LibBase);
}

BPTR
_LibExpunge(struct Library * _base);

BPTR
gwLibExpunge(struct LibraryManagerInterface* _iface)
{
  return _LibExpunge((struct Library *) _iface->Data.LibBase);
}

ULONG
_LibNull(struct Library * _base);

ULONG
gwLibNull(struct LibraryManagerInterface* _iface)
{
  return _LibNull((struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_AllocAudio(struct TagItem * ___tagList, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_AllocAudio(struct AHIsubIFace* _iface,
	struct TagItem * ___tagList,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_AllocAudio(___tagList, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

void
_AHIsub_FreeAudio(struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

void
gwAHIsub_FreeAudio(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_FreeAudio(___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

void
_AHIsub_Disable(struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

void
gwAHIsub_Disable(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_Disable(___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

void
_AHIsub_Enable(struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

void
gwAHIsub_Enable(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_Enable(___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_Start(ULONG ___Flags, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_Start(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_Start(___Flags, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_Update(ULONG ___Flags, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_Update(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_Update(___Flags, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_Stop(ULONG ___Flags, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_Stop(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_Stop(___Flags, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_SetVol(UWORD ___Channel, Fixed ___Volume, sposition ___Pan, struct AHIAudioCtrlDrv * ___AudioCtrl, ULONG ___Flags, struct Library * _base);

ULONG
gwAHIsub_SetVol(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	Fixed ___Volume,
	sposition ___Pan,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags)
{
  return _AHIsub_SetVol(___Channel, ___Volume, ___Pan, ___AudioCtrl, ___Flags, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_SetFreq(UWORD ___Channel, ULONG ___Freq, struct AHIAudioCtrlDrv * ___AudioCtrl, ULONG ___Flags, struct Library * _base);

ULONG
gwAHIsub_SetFreq(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	ULONG ___Freq,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags)
{
  return _AHIsub_SetFreq(___Channel, ___Freq, ___AudioCtrl, ___Flags, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_SetSound(UWORD ___Channel, UWORD ___Sound, ULONG ___Offset, LONG ___Length, struct AHIAudioCtrlDrv * ___AudioCtrl, ULONG ___Flags, struct Library * _base);

ULONG
gwAHIsub_SetSound(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	UWORD ___Sound,
	ULONG ___Offset,
	LONG ___Length,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags)
{
  return _AHIsub_SetSound(___Channel, ___Sound, ___Offset, ___Length, ___AudioCtrl, ___Flags, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_SetEffect(APTR ___Effect, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_SetEffect(struct AHIsubIFace* _iface,
	APTR ___Effect,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_SetEffect(___Effect, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_LoadSound(UWORD ___Sound, ULONG ___Type, APTR ___Info, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

ULONG
gwAHIsub_LoadSound(struct AHIsubIFace* _iface,
	UWORD ___Sound,
	ULONG ___Type,
	APTR ___Info,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_LoadSound(___Sound, ___Type, ___Info, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

ULONG
_AHIsub_UnloadSound(UWORD ___Sound, struct AHIAudioCtrlDrv * ___Audioctrl, struct Library * _base);

ULONG
gwAHIsub_UnloadSound(struct AHIsubIFace* _iface,
	UWORD ___Sound,
	struct AHIAudioCtrlDrv * ___Audioctrl)
{
  return _AHIsub_UnloadSound(___Sound, ___Audioctrl, (struct Library *) _iface->Data.LibBase);
}

LONG
_AHIsub_GetAttr(ULONG ___Attribute, LONG ___Argument, LONG ___DefValue, struct TagItem * ___tagList, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

LONG
gwAHIsub_GetAttr(struct AHIsubIFace* _iface,
	ULONG ___Attribute,
	LONG ___Argument,
	LONG ___DefValue,
	struct TagItem * ___tagList,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_GetAttr(___Attribute, ___Argument, ___DefValue, ___tagList, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}

LONG
_AHIsub_HardwareControl(ULONG ___Attribute, LONG ___Argument, struct AHIAudioCtrlDrv * ___AudioCtrl, struct Library * _base);

LONG
gwAHIsub_HardwareControl(struct AHIsubIFace* _iface,
	ULONG ___Attribute,
	LONG ___Argument,
	struct AHIAudioCtrlDrv * ___AudioCtrl)
{
  return _AHIsub_HardwareControl(___Attribute, ___Argument, ___AudioCtrl, (struct Library *) _iface->Data.LibBase);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
