#include "btaudio.h"
#include "audio_vectors.h"

AROS_LH1(struct BluetoothAudioBase *, audioOpen, AROS_LHA(ULONG, version, D0),
    struct BluetoothAudioBase *, base, 1, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    base->library.lib_OpenCnt++;
    return base;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(BPTR, audioClose, struct BluetoothAudioBase *, base, 2, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    base->library.lib_OpenCnt--;
    return BNULL;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(BPTR, audioExpunge, struct BluetoothAudioBase *, base, 3, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    /* The class owns this sublibrary and expunges it with its units. */
    return BNULL;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, audioReserved, struct BluetoothAudioBase *, base, 4, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
static const APTR vectors[] = {
    &AROS_SLIB_ENTRY(audioOpen, btaudio_sub, 1),
    &AROS_SLIB_ENTRY(audioClose, btaudio_sub, 2),
    &AROS_SLIB_ENTRY(audioExpunge, btaudio_sub, 3),
    &AROS_SLIB_ENTRY(audioReserved, btaudio_sub, 4),
    &AROS_SLIB_ENTRY(sub_AllocAudio, btaudio_sub, 5),
    &AROS_SLIB_ENTRY(sub_FreeAudio, btaudio_sub, 6),
    &AROS_SLIB_ENTRY(sub_Disable, btaudio_sub, 7),
    &AROS_SLIB_ENTRY(sub_Enable, btaudio_sub, 8),
    &AROS_SLIB_ENTRY(sub_Start, btaudio_sub, 9),
    &AROS_SLIB_ENTRY(sub_Update, btaudio_sub, 10),
    &AROS_SLIB_ENTRY(sub_Stop, btaudio_sub, 11),
    &AROS_SLIB_ENTRY(sub_SetVol, btaudio_sub, 12),
    &AROS_SLIB_ENTRY(sub_SetFreq, btaudio_sub, 13),
    &AROS_SLIB_ENTRY(sub_SetSound, btaudio_sub, 14),
    &AROS_SLIB_ENTRY(sub_SetEffect, btaudio_sub, 15),
    &AROS_SLIB_ENTRY(sub_LoadSound, btaudio_sub, 16),
    &AROS_SLIB_ENTRY(sub_UnloadSound, btaudio_sub, 17),
    &AROS_SLIB_ENTRY(sub_GetAttr, btaudio_sub, 18),
    &AROS_SLIB_ENTRY(sub_HardwareControl, btaudio_sub, 19),
    (APTR)-1
};

struct BluetoothAudioBase *bAudioCreate(struct BTAudioClass *cls)
{
    struct BluetoothAudioBase *base = (APTR)MakeLibrary((APTR)vectors, NULL, NULL, sizeof(*base), BNULL);
    if(base) {
        base->cls = cls;
        base->library.lib_Node.ln_Type = NT_LIBRARY;
        base->library.lib_Node.ln_Name = LibName;
        base->library.lib_Version = 6;
        base->library.lib_Revision = 1;
        base->library.lib_IdString = LibIDString;
        base->library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
        base->library.lib_OpenCnt = 1;
        AddLibrary(&base->library);
    }
    return base;
}
void bAudioDelete(struct BluetoothAudioBase *base)
{
    Remove((struct Node *)base);
    FreeMem((UBYTE *)base - base->library.lib_NegSize,
        base->library.lib_NegSize + base->library.lib_PosSize);
}
