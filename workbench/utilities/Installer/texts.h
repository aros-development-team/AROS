#ifndef _TEXTS_H
#define _TEXTS_H

/* Preset @variables */
struct CommandList
{
  char * varsymbol;
  char * vartext;
  int varinteger;
};


struct CommandList commands[] =
{
  {"abort-button",	"Abort",	0},
  {"app-name",		"",	0},
  {"askchoice-help",	"",	0},
  {"askdir-help",	"",	0},
  {"askdisk-help",	"",	0},
  {"askfile-help",	"",	0},
  {"asknumber-help",	"",	0},
  {"askoptions-help",	"",	0},
  {"askstring-help",	"",	0},
  {"copyfiles-help",	"",	0},
  {"copylib-help",	"",	0},
  {"default-dest",	"",	0},
  {"each-name",		"",	0},
  {"each-type",		"",	0},
  {"error-msg",		"",	0},
  {"execute-dir",	"",	0},
  {"icon",		"",	0},
  {"ioerr",		"",	0},
  {"language",		"",	0},
  {"makedir-help",	"",	0},
  {"pretend",		"",	0},
  {"special-msg",	"",	0},
  {"startup-help",	"",	0},
  {"user-level",	"",	0},

  {"",			NULL,	0 }
};

#endif /* _TEXTS_H */
