#ifndef _TEXTS_H
#define _TEXTS_H

/* Preset variables */

#define ABORT_BUTTON	"Abort Install"
#define DEFAULT_DEST	"ProgDir"


/* Texts */

#define USERLEVEL_REQUEST "Which user-level do you want?"
#define NOVICE_NAME	"Novice"
#define ADVANCED_NAME	"Advanced"
#define EXPERT_NAME	"Expert"
#define HELP_ON_USERLEVEL "Help on UserLevel"
#define USERLEVEL_HELP	"
 NOVICE  won't be asked anymore questions
 AVERAGE will have to interact sometimes
 EXPERT  must confirm all actions
"

#define LOG_QUESTION	"Installer can log all actions. What do you want?"
#define LOG_FILE_TEXT	"Write to log file"
#define LOG_PRINT_TEXT	"Log to printer (PRT:)"
#define LOG_NOLOG_TEXT	"Disable logging"
#define HELP_ON_LOGFILES "Help on Logfiles"
#define LOG_HELP	"
 Printer will go to PRT:
 Log File will be %s
"

#define PRETEND_QUESTION "Installer can pretend to install. Do you want this?"
#define NOPRETEND_TEXT	"Install for real"
#define PRETEND_TEXT	"Pretend to install"
#define HELP_ON_PRETEND	"Help on Pretend-to-install mode"
#define PRETEND_HELP	"
 \"Pretend to Install\" will not install the
 application for real on your Disk, instead
 it will only log all the actions to the
 log-file.  However if the \"(safe)\" flag is
 set for functions in the script these will
 be done even if in pretend mode.
"

#define HELP_ON_MESSAGE "Help on Message"
#define ABOUT_ON_INSTALLER "About Installer"
#define ABOUT_INSTALLER	"
 This is AROS Installer V%d.%d
 It is intended to be compatible to

   Installer V43.3


 This program was written by

   Henning Kiel <hkiel@aros.org>
"


/* Miscellaneous strings */

#define EMPTY_STRING	""


#endif /* _TEXTS_H */

