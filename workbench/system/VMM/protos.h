/* $Id: protos.h,v 3.7 95/12/16 18:37:10 Martin_Apel Exp $ */

/* bitmap.c */
int InitMap (ULONG);
void KillMap (void);
ULONG AllocSlotNextTo (ULONG);
void FreeSlot (ULONG);
ULONG AllocMultipleSlots (ULONG*);

/* cache.c */
int InitCache (void);
void KillCache (void);
BOOL ReadPage (ULONG , struct TrapStruct*);
BOOL WritePage (ULONG , struct TrapStruct*);
ULONG AllocPageOnDisk (void);
void FreePageOnDisk (ULONG);
ULONG SlotsUsed (void);
void AllocNewCache (void);
void NewCache (char *buffer, ULONG size);

/* config.c */
int ReadConfigFile(char *name);

/* debug.c */
#ifdef DEBUG
BOOL OpenDebugWindow(void);
void CloseDebugWindow(void);
void PrintDebugMsg(char * , long );
void PrintTrapStruct(struct TrapStruct * );
void CheckMemList (void);
#endif

/* fault.c */
void AddFree (struct TrapStruct *ThisFault);
void AddPageReq (struct TrapStruct *ThisFault);
void CheckWaitingFaults(void);
void HandlePageFault(void);
void ReadReturned (struct TrapStruct *ThisFault);
void WriteReturned (struct TrapStruct *ThisFault);
BOOL AddFrame (ULONG priority);
void RemAllFrames (void);
void HandleFreeMsg (void);

/* ffs.c */
BOOL IsPseudoPart (ULONG header_block, ULONG *first_block, ULONG *last_block);
int CreatePseudoPart (ULONG header_block, ULONG *first_block, ULONG *last_block);
int IsValidFFSPartition (void);
int GetDiskType (char *name, ULONG *DiskType);

/* find_dev_params.c */
BOOL FindDevParams(char * , struct DOSDevParams *);
BOOL GetPartName  (char *, char *);

/* forbidden_tasks.c */
void ExtCheckVirtMem(struct Task *AskingTask);
BOOL CodePagingAllowed (const char *filename);
int EnterTask(const char * , ULONG, ULONG, BOOL, BOOL);
BOOL RemoveTask (const char*);
void NoMoreVM (void);
int InitTaskTable (void);
void KillTaskTable (void);

/* globals.c */
void InitError (int);
void RunTimeError (int);
void FatalError (int);
void ReportError (const char *error_string, UWORD error_level);
void *AllocAligned (ULONG size, ULONG flags, ULONG alignment, ULONG priority);
void GetNthString (char *from, char *to, int n);
void StrToHex (char *string, ULONG *val);
void EmptyPageCollector (void);

/* mem_tracking.c */
int InitTrackInfo (void);
void KillTrackInfo (void);
ULONG *CreateTrackInfo (ULONG *buffer, ULONG orig_size);
void ChangeOwner (ULONG *buffer);
void FreeTrackInfo (void *TrackBuffer);
void VMUsageInfo (struct VMMsg *UsageMsg);

/* mmu_table.c */
int MarkAddress (IPTR start, ULONG length, ULONG type, IPTR phys_addr,
                 BOOL Small);
int SetupMMUTable (void);
BOOL KillPageTable (ULONG *pt, BOOL FreePages);
BOOL KillPointerTable (ULONG *pt, BOOL FreePages);
void KillMMUTable (void);
void AllowZorroIICaching (BOOL Allowed);
int AllocAddressRange (void);
int SwitchFastROM (BOOL On);
void MarkPage (IPTR addr, ULONG CacheMode);

/* pagehandler.c */
void PageHandler (void);

/* pageio.c */
int OpenPageFile(void);
void ClosePageFile(void);
void WriteSinglePage(ULONG , struct TrapStruct * );
void ReadSinglePage(ULONG , struct TrapStruct * );
void WriteMultiplePages (ULONG, void*, ULONG);
void HandleReturn(void);

/* prepager.c */
void PrePager(void);

/* reset_handler.c */
void *InstallResetHandler (void (*func) (void), LONG priority);
void RemoveResetHandler (void* ResetHandlerParams);
void ResetHandlerDone (void *ResetHandlerParams);

/* stat.c */
void Statistics (void);

/* timer.c */
void *InitTimer (int, UWORD *);
void CloseTimer (void *);
BOOL AddTimedFunction (void *, ULONG secs, ULONG micros, void (*function) ());
void HandleTimerReturn (void *);

/* VM_Manager.c */
void VM_Manager (void);

/* mem_trace.asm */
void AllocVM (void);
void AllocMemPatch (void);
void FreeMemPatch (void);
void AvailMemPatch (void);
APTR DoOrigAllocMem (IPTR, ULONG);
IPTR DoOrigAvailMem (ULONG);

/* parthandler.asm */
void PartHandler (void);

/* sv_regs40.asm */
long ReadVBR (void);
void CPushP40 (ULONG address);
void CPushL40 (ULONG address);
void PFlushP40 (ULONG address);
void PFlushA40 (void);
void ReadMMUState40 (struct MMUState40*);
void SetMMUState40 (struct MMUState40*);
void SaveMMUState40 (void);
void RestoreMMUState40 (void);
ULONG GenDescr40 (ULONG LogicalAddr);
ULONG GetPageSize40 (void);
void EmptyFunc (void);

/* sv_regs30.asm */
long ReadVBR (void);
void PFlushA30( void );
void PFlushP30( ULONG address );
void ReadMMUState30 (struct MMUState30*);
void SetMMUState30 (struct MMUState30*);
void SaveMMUState30 (void);
void RestoreMMUState30 (void);
ULONG GenDescr30 (ULONG LogicalAddr);
void ColdRebootPatch (void);
ULONG GetPageSize30 (void);

/* sv_regs851.asm */
BOOL MMU68851 (void);
void ReadMMUState851 (struct MMUState30*);
void SetMMUState851 (struct MMUState30*);
void SaveMMUState851 (void);
void RestoreMMUState851 (void);
ULONG GenDescr851 (ULONG LogicalAddr);

/* sv_regs60.asm */
BOOL Is68060 (void);
void CPushP60 (ULONG address);
void CPushL60 (ULONG address);

#define PFlushP60 PFlushP40
#define PFlushA60 PFlushA40
#define GetPageSize60 GetPageSize40
#define GenDescr60 GenDescr40

/* switch_patch.asm */
void SwitchPatch (void);
void AddTaskPatch (void);
void WaitPatch (void);
#ifdef DEBUG
void RemTaskPatch (void);
void OpenPatch (void);
void StackSwapPatch (void);
#endif

/* traphandler60.asm */
void TrapHandler60(void);
void DynMMUTrap60 (void);

/* traphandler40.asm */
void TrapHandler40(void);
void DynMMUTrap40 (void);

/* traphandler30.asm */
void TrapHandler30(void);
void DynMMUTrap30 (void);

BOOL VMMInstallTrapHandler(void);
void VMMRemoveTrapHandler(void);

/* loadseg_patch.asm */
void LoadSegPatch (void);
void NewLoadSegPatch (void);
#ifdef DEBUG
void CrashHandler (void);
void FindHunk (IPTR address);
void AlertPatch (void);
#endif

/* dma_patch.asm */
void CachePreDMAPatch (void);
void CachePostDMAPatch (void);

/* wb_patch.asm */
void SetWindowTitlesPatch (void);

/* semaphores.asm */
void VMMObtainSemaphore (struct SignalSemaphore*);
void VMMReleaseSemaphore (struct SignalSemaphore*);
void VMMInitSemaphore (struct SignalSemaphore*);

/* external definitions used are put here, so we don't have to include
 * stdio.h.
 */
int sprintf(char *, char *, ...);
