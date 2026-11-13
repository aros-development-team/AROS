#ifndef MANAGER_PRIV
#define MANAGER_PRIV extern
#define NO_INIT
#endif

MANAGER_PRIV BOOL EarlyExit
#ifndef NO_INIT
   = FALSE
#endif
;

MANAGER_PRIV BOOL VMEnabled;
MANAGER_PRIV struct NotifyRequest CfgChangeNotify;
MANAGER_PRIV BYTE CfgChangeSignal;
MANAGER_PRIV struct ExtPort *ExtCxPort;
MANAGER_PRIV LONG CxSignal;

struct RexxFuncEntry
  {
  char *FuncName;
  UWORD FuncNum;
  };

enum
  {
  REXX_INV_CMD,
  REXX_ADDPROG,
  REXX_REMPROG,
  REXX_ENABLE,
  REXX_DISABLE,
  REXX_QUIT,
  REXX_HIDE,
  REXX_SHOW,
  REXX_INFO,
  REXX_STAT,
  REXX_MEM,
  REXX_MEMTYPE,
  REXX_WRITEBUFFER,
  REXX_VMPRI,
  REXX_PATCHWB,
  REXX_MEMTRACK,
  REXX_MINVMALLOC,
  REXX_HELP,
  REXX_ZOOM
  };

MANAGER_PRIV struct RexxFuncEntry RexxFuncs[]
#ifndef NO_INIT
 = {
    { "ADD_PROG",    REXX_ADDPROG     },
    { "REM_PROG",    REXX_REMPROG     },
    { "ENABLE",      REXX_ENABLE      },
    { "DISABLE",     REXX_DISABLE     },
    { "QUIT",        REXX_QUIT        },
    { "HIDE",        REXX_HIDE        },
    { "SHOW",        REXX_SHOW        },
    { "STAT",        REXX_STAT        },
    { "INFO",        REXX_INFO        },
    { "MEM",         REXX_MEM         },
    { "MEMTYPE",     REXX_MEMTYPE     },
    { "WRITEBUFFER", REXX_WRITEBUFFER },
    { "VMPRIORITY",  REXX_VMPRI       },
    { "PATCHWB",     REXX_PATCHWB     },
    { "MEMTRACK",    REXX_MEMTRACK    },
    { "MINVMALLOC",  REXX_MINVMALLOC  },
    { "HELP",        REXX_HELP        },
    { "ZOOM",        REXX_ZOOM        },
    
    { NULL,          0             }
  }
#endif
;

#undef NO_INIT

/* A few prototypes only used by the VM_Manager */
int Init_VM_Manager (void);
void Cleanup_VM_Manager (void);
BOOL HandleRexxMsg (void);
BOOL TryToQuit (void);
void FillStat (struct VMMsg *StatMsg);
int ShowGUI (void);
void HideGUI (void);
int LaunchStat (void);
