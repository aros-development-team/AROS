/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _GUI_H
#define _GUI_H

extern void show_abort(char *msg);
extern void show_complete(long int percent);
extern void show_exit(char *msg);
extern void show_parseerror(char * msg, int errline);
extern void show_working(char *msg);
extern void show_message(char * msg ,struct ParameterList * pl);
extern void request_userlevel(char *msg);
extern long int request_bool(struct ParameterList *pl);
extern long int request_number(struct ParameterList *pl);
extern char *request_string(struct ParameterList *pl);
extern long int request_choice(struct ParameterList *pl);
extern char *request_dir(struct ParameterList *pl);
extern char *request_disk(struct ParameterList *pl);
extern char *request_file(struct ParameterList *pl);
extern long int request_options(struct ParameterList *pl);
extern int request_confirm(struct ParameterList * pl);
extern void final_report();
extern void display_text(char * msg);
extern void init_gui();
extern void deinit_gui();

#endif /* _GUI_H */
