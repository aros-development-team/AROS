AROS_LD2(ULONG, sub_AllocAudio,
	AROS_LDA(struct TagItem *, taglist, A1),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 5, btaudio_sub);
AROS_LD1(void, sub_FreeAudio,
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 6, btaudio_sub);
AROS_LD1(void, sub_Disable,
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 7, btaudio_sub);
AROS_LD1(void, sub_Enable,
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 8, btaudio_sub);
AROS_LD2(ULONG, sub_Start,
	AROS_LDA(ULONG, flags, D0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 9, btaudio_sub);
AROS_LD2(void, sub_Update,
	AROS_LDA(ULONG, flags, D0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 10, btaudio_sub);
AROS_LD2(void, sub_Stop,
	AROS_LDA(ULONG, flags, D0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 11, btaudio_sub);
AROS_LD5(IPTR, sub_GetAttr,
	AROS_LDA(ULONG, attribute, D0),
	AROS_LDA(LONG, argument, D1),
	AROS_LDA(IPTR, def, D2),
	AROS_LDA(struct TagItem *, taglist, A1),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 18, btaudio_sub);
AROS_LD3(ULONG, sub_HardwareControl,
	AROS_LDA(ULONG, attribute, D0),
	AROS_LDA(LONG, argument, D1),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 19, btaudio_sub);
AROS_LD5(ULONG, sub_SetVol,
	AROS_LDA(UWORD, channel, D0),
	AROS_LDA(Fixed, volume, D1),
	AROS_LDA(sposition, pan, D2),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LDA(ULONG, flags, D3),
	struct BluetoothAudioBase *, AHIsubBase, 12, btaudio_sub);
AROS_LD4(ULONG, sub_SetFreq,
	AROS_LDA(UWORD, channel, D0),
	AROS_LDA(ULONG, freq, D1),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LDA(ULONG, flags, D2),
	struct BluetoothAudioBase *, AHIsubBase, 13, btaudio_sub);
AROS_LD6(ULONG, sub_SetSound,
	AROS_LDA(UWORD, channel, D0),
	AROS_LDA(UWORD, sound, D1),
	AROS_LDA(ULONG, offset, D2),
	AROS_LDA(LONG, length, D3),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LDA(ULONG, flags, D4),
	struct BluetoothAudioBase *, AHIsubBase, 14, btaudio_sub);
AROS_LD2(ULONG, sub_SetEffect,
	AROS_LDA(APTR, effect, A0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 15, btaudio_sub);
AROS_LD4(ULONG, sub_LoadSound,
	AROS_LDA(UWORD, sound, D0),
	AROS_LDA(ULONG, type, D1),
	AROS_LDA(APTR, info, A0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 16, btaudio_sub);
AROS_LD2(ULONG, sub_UnloadSound,
	AROS_LDA(UWORD, sound, D0),
	AROS_LDA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 17, btaudio_sub);
