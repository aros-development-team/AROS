/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _TEXTS_H
#define _TEXTS_H

/* Preset variables */

#define ABORT_BUTTON	"Abort Install"
#define DEFAULT_DEST	"ProgDir"


/* Texts */

#define WELCOME_TEMPLATE "Welcome to the %s App installation utility!"
#define USERLEVEL_REQUEST "Which user-level do you want?"
#define NOVICE_NAME	"Novice"
#define ADVANCED_NAME	"Advanced"
#define EXPERT_NAME	"Expert"
#define HELP_ON_USERLEVEL "Help on UserLevel"
#define USERLEVEL_HELP				\
" Please select the installation method\n"	\
" according to your knowledge.\n"		\
"\n"						\
"   NOVICE\n"					\
"     user won't be asked anymore questions.\n"	\
"     However, the user may be requested to\n"	\
"     insert disks or make other decisions.\n"	\
"\n"						\
"   AVERAGE\n"					\
"     user will have to interact sometimes.\n"	\
"     This usually includes important\n"	\
"     decisions, but not unportant ones like\n"	\
"     copying files or creating directories.\n"	\
"\n"						\
"   EXPERT\n"					\
"     user must confirm all actions.\n"

#define MESSAGE		"Message"
#define PROMPT		"Prompt"
#define YES_TEXT	"Yes"
#define NO_TEXT		"No"
#define LOG_QUESTION	"Installer can log all actions. What do you want?"
#define LOG_FILE_TEXT	"Write to log file"
#define LOG_PRINT_TEXT	"Log to printer (PRT:)"
#define LOG_NOLOG_TEXT	"Disable logging"
#define HELP_ON_LOGFILES "Help on Logfiles"
#define LOG_HELP		\
" Printer will go to PRT:\n"	\
" Log File will be %s"

#define PRETEND_QUESTION "Installer can pretend to install. Do you want this?"
#define NOPRETEND_TEXT	"Install for real"
#define PRETEND_TEXT	"Pretend to install"
#define HELP_ON_PRETEND	"Help on Pretend-to-install mode"
#define PRETEND_HELP					\
" \"Pretend to Install\" will not install the\n"	\
" application for real on your Disk, instead\n"		\
" it will only log all the actions to the\n"		\
" log-file.  However, if the \"(safe)\" flag is\n"	\
" set for functions in the script these will\n"		\
" be done even if in pretend mode."

#define HELP_ON_MESSAGE "Help on Message"
#define ABOUT_ON_INSTALLER "About Installer"
#define ABOUT_INSTALLER			\
" This is AROS Installer V%d.%d\n"	\
" Copyright © 1995-2003, The AROS Development Team.\n"\
" All rights reserved.\n"		\
"\n"					\
" It is intended to be compatible to\n"	\
"\n"					\
"        Installer V43.3\n"		\
"\n"					\
"\n"					\
" This program was mainly written by\n"	\
"\n"					\
"        Henning Kiel <hkiel@aros.org>\n"


#define HELP_ON_USERCONFIRM	"Help on Confirmation"
#define USERCONFIRM_HELP			\
" Click \"Proceed\" button to perform action\n"	\
" or \"Skip\" button to leave it out."


#define HELP_ON_CONFIRM "Help on Confirmation"

#define HELP_ON_ASKCHOICE "Help on AskChoice"
#define ASKCHOICE_HELP					\
" Select one of the specified items. Proceed\n"		\
" with your selected item with \"Proceed\" or\n"	\
" abort the installation with \"Abort\"."


#define HELP_ON_ASKOPTIONS "Help on AskOptions"
#define ASKOPTIONS_HELP					\
" Select all items you want or need. Proceed\n"		\
" with your selected items with \"Proceed\" or\n"	\
" abort the installation with \"Abort\"."


#define HELP_ON_ASKNUMBER "Help on AskNumber"
#define ASKNUMBER_HELP					\
" Fill in the number gadget and press \"Proceed\"\n"	\
" to proceed or abort the installation with\n"		\
" \"Abort\"."


#define HELP_ON_ASKSTRING "Help on AskString"
#define ASKSTRING_HELP					\
" Fill in the string gadget and press \"Proceed\"\n"	\
" to proceed or abort the installation with\n"		\
" \"Abort\"."



/* Miscellaneous strings */

#define EMPTY_STRING	""


#endif /* _TEXTS_H */

