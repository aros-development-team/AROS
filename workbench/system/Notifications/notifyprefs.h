#ifndef NOTIFYPREFS_H
#define NOTIFYPREFS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Notifications settings: a PrefsObjects dictionary in
          ENV(ARC):Sys/Notifications.xml, shared by the server and
          Prefs/Notifications.
*/

#define NPREFS_FILE         "Sys/Notifications.xml"
#define NPREFS_APPNAME      "Notifications"
#define NPREFS_URL          "aros.org"
#define NPREFS_EDITOR       "SYS:Prefs/Notifications"
#define NPREFS_LOGFILE      "T:Notifications.log"

#define NPK_POSITION        "Position"      /* string, one of NPOS_ names       */
#define NPK_DISPLAYTIME     "DisplayTime"   /* integer s, 0 = by text length    */
#define NPK_MAXVISIBLE      "MaxVisible"    /* integer, more wait their turn    */
#define NPK_WIDTH           "PopupWidth"    /* integer pixels                   */
#define NPK_HOLDAWAY        "HoldWhileAway" /* bool: no countdown while idle    */
#define NPK_DND             "DoNotDisturb"  /* bool: log only, but priority 10  */
#define NPK_BEEP            "BeepOnUrgent"  /* bool: DisplayBeep() above pri 8  */
#define NPK_SOUNDS          "PlaySounds"    /* bool                             */
#define NPK_DEFSOUND        "DefaultSound"  /* string, played for every pop-up  */
#define NPK_LOG             "Log"           /* bool: append to NPREFS_LOGFILE   */
#define NPK_APPS            "Applications"  /* dict: name -> bool, FALSE = none */

#define NPOS_BOTTOMRIGHT    0
#define NPOS_BOTTOMLEFT     1
#define NPOS_TOPRIGHT       2
#define NPOS_TOPLEFT        3

#define NPREFS_POSITIONS    { "BottomRight", "BottomLeft", "TopRight", "TopLeft", NULL }

#define NDEF_POSITION       NPOS_TOPRIGHT
#define NDEF_DISPLAYTIME    0
#define NDEF_MAXVISIBLE     5
#define NDEF_WIDTH          320
#define NDEF_HOLDAWAY       TRUE
#define NDEF_DND            FALSE
#define NDEF_BEEP           TRUE
#define NDEF_SOUNDS         TRUE
#define NDEF_LOG            TRUE

#define NLIM_DISPLAYTIME    60
#define NLIM_MAXVISIBLE     10
#define NLIM_WIDTHMIN       200
#define NLIM_WIDTHMAX       640

#endif /* NOTIFYPREFS_H */
