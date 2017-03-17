/*
     AHI - The AHI preferences program
     Copyright (C) 2017 The AROS Dev Team
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _AHI_H_
#define _AHI_H_

#include <exec/types.h>
#include <devices/ahi.h>

struct UnitNode {
  struct Node           node;
  char                  name[32];
  struct AHIUnitPrefs   prefs;
};

struct ModeNode {
  struct Node           node;
  IPTR                 ID;
  char                  name[80];
};

#define HELPFILE    "ahi.guide"
#define ENVARCFILE  "ENVARC:Sys/ahi.prefs"
#define ENVFILE     "ENV:Sys/ahi.prefs"

struct state 
{
  IPTR UnitSelected;
  IPTR ModeSelected;
  IPTR FreqSelected;
  IPTR ChannelsSelected;
  IPTR InputSelected;
  IPTR OutputSelected;
  IPTR OutVolSelected;
  IPTR MonVolSelected;
  IPTR GainSelected;

  IPTR Frequencies;
  IPTR Channels;
  IPTR Inputs;
  IPTR Outputs;
  IPTR OutVols;
  IPTR MonVols;
  IPTR Gains;

  BOOL ChannelsDisabled;
  BOOL OutVolMute;
  BOOL MonVolMute;
  BOOL GainMute;

  float OutVolOffset;
  float MonVolOffset;
  float GainOffset;
};

struct args
{
  STRPTR  from;
  IPTR   edit;
  IPTR   use;
  IPTR   save;
  STRPTR  pubscreen;
};

extern char const *Version;

extern struct List *UnitList;
extern struct List *ModeList;

extern char **Units;
extern char **Modes;
extern char **Inputs;
extern char **Outputs;


extern struct state state;
extern struct args args;

extern BOOL SaveIcons;

void NewSettings(char * );
void NewUnit(int );
void NewMode(int );

void FillUnit(void);

char *getFreq(void);
char *getChannels(void);
char *getOutVol(void);
char *getMonVol(void);
char *getGain(void);
char *getInput(void);
char *getOutput(void);
IPTR getAudioMode(void);
char *getRecord(void);
char *getAuthor(void);
char *getCopyright(void);
char *getDriver(void);
char *getVersion(void);
char *getAnnotation(void);

#endif /* _AHI_H_ */

