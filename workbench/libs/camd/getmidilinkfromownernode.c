/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/


#include "camd_intern.h"


struct MidiLink *GetMidiLinkFromOwnerNode(struct MinNode *node){
	struct MidiLink dummy;
	return (struct MidiLink *)((char *)((char *)(node)-((char *)&dummy.ml_OwnerNode-(char *)&dummy)));
}

