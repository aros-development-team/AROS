#ifndef LIBRARIES_SCREENNOTIFY_H
#define LIBRARIES_SCREENNOTIFY_H

/*
 * libraries/screennotify_protos.h
 *
 * Include file for screennotify.library
 *
 * $VER: screennotify.h 1.0 (26.03.95)
 *
 */

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

/* Name and version */
#define SCREENNOTIFY_NAME    "screennotify.library"
#define SCREENNOTIFY_VERSION 1

/* Message sent to clients */
struct ScreenNotifyMessage {
 struct Message snm_Message;
 ULONG          snm_Type;    /* READ ONLY!! */
 APTR           snm_Value;   /* READ ONLY!! */
};

/* Values for snm_Type */
#define SCREENNOTIFY_TYPE_CLOSESCREEN   0 /* CloseScreen() called, snm_Value contains */
                                          /* pointer to Screen structure              */
#define SCREENNOTIFY_TYPE_PUBLICSCREEN  1 /* PubScreenStatus() called to make screen  */
                                          /* public, snm_Value contains pointer to    */
                                          /* PubScreenNode structure                  */
#define SCREENNOTIFY_TYPE_PRIVATESCREEN 2 /* PubScreenStatus() called to make screen  */
                                          /* private, snm_Value contains pointer to   */
                                          /* PubScreenNode structure                  */
#define SCREENNOTIFY_TYPE_WORKBENCH     3 /* snm_Value == FALSE (0): CloseWorkBench() */
                                          /* called, please close windows on WB       */
                                          /* snm_Value == TRUE  (1): OpenWorkBench()  */
                                          /* called, windows can be opened again      */
#endif
