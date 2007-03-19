struct DriveControls {
	Object *NoClickSwitch;
	Object *RetriesSlider;
	char DriveLabel[64];
	ULONG Disabled;
};

struct WindowGroup {
	struct TagItem DriveGroup[TD_NUMUNITS];
	ULONG TagChild;
	Object *ButtonsGroup;
	ULONG TagDone;
};

struct TDU_Prefs {
	ULONG TagUnitNum;
	ULONG Unit;
	ULONG TagPubFlags;
	ULONG PubFlags;
	ULONG TagRetryCnt;
	ULONG RetryCnt;
};

struct TrackdiskPrefs {
	struct TDU_Prefs UnitPrefs[TD_NUMUNITS];
	ULONG TagDone;
};

ULONG Main(void);
Object *CreateDriveControls(struct DriveControls *dc, int ndrive);
void InitUnitPrefs(struct TDU_Prefs *UnitPrefs, int nunit);
void LoadPrefs(void);
void ControlsToPrefs(struct DriveControls *dc, struct TDU_Prefs *pr);
void SavePrefs(void);
void UsePrefs(void);

