/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: DataTypesDescriptorCreator
    Lang: English.
*/

/*
 *  includes
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <c_iff.h>

#include "dtdesc.h"

/*
 *  defines
 */

#define READBUFFERSIZE (256)

/*
 *  structs
 */

struct DTDesc
{
 char *ProgName;
 char *InputName;
 char *OutputName;
 FILE *Input;

 struct DataTypeHeader DTH;
 CARD8  Name[READBUFFERSIZE];
 CARD8  Version[READBUFFERSIZE];
 CARD8  BaseName[READBUFFERSIZE];
 CARD8  Pattern[READBUFFERSIZE];
 CARD16 Mask[READBUFFERSIZE];

 CARD8 ReadBuffer[READBUFFERSIZE];
};

/*
 *  prototypes
 */

int main(int argc, char **argv);
int Init(int argc, char **argv, struct DTDesc **TheDTDesc);
void Work(struct DTDesc *TheDTDesc);
void Cleanup(struct DTDesc *TheDTDesc);

void Usage(char *ProgName);
int ParseArgs(int argc, char **argv, struct DTDesc *TheDTDesc);
int OpenInput(struct DTDesc *TheDTDesc);

int HandleLine(struct DTDesc *TheDTDesc);
int HandleName(struct DTDesc *TheDTDesc);
int HandleVersion(struct DTDesc *TheDTDesc);
int HandleBaseName(struct DTDesc *TheDTDesc);
int HandlePattern(struct DTDesc *TheDTDesc);
int HandleMask(struct DTDesc *TheDTDesc);
int HandleGroupID(struct DTDesc *TheDTDesc);
int HandleID(struct DTDesc *TheDTDesc);
int HandleFlags(struct DTDesc *TheDTDesc);
int HandlePriority(struct DTDesc *TheDTDesc);

int WriteOutDTD(struct DTDesc *TheDTDesc);

