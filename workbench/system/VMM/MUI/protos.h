BOOL CreateGUI (void);
void DisposeGUI (void);
Object *MyButton (IPTR label, IPTR shortcut);
Object *CreateTaskPage (void);
Object *CreateTaskSelectWindow (void);
Object *CreateMemoryPage (void);
Object *CreateMiscPage (void);
int HandleGUI (void);
int main (LONG argc, UBYTE **argv);

BOOL StartVMM (void);
BOOL InstallAsCommodity (void);
void ReadConfigFromVMM (void);
BOOL ReadConfigFileMUI (char *name);
void ReadHotkeysFromConfig (char *name);
BOOL WriteConfigFile (char *name);
BOOL StopVMM (void);

int HandleCxMsg (void);
void UninstallAsCommodity (void);
void HandleDiskPopup ( void);
void BuildTaskList (void);

/* Menu functions */
BOOL MenuOpen (void);
BOOL MenuSaveAs (void);
BOOL MenuSaveWin (void);
BOOL MenuAbout (void);
BOOL MenuHide (void);
/* Quit needs no function */
BOOL MenuDefault (void);
BOOL HandleMenuChoice (ULONG id);

void AddNamed (char *name);
BOOL ValidSettings (void);

void InitHook (struct Hook *hook, HOOKFUNC func);
ULONG MaxAvailMem (ULONG flags);
