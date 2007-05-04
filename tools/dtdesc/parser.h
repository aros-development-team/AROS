/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DataTypesDescriptorCreator.
    Lang: English.
*/

enum Keywords
{
 Name,
 Version,
 BaseName,
 Pattern,
 Mask,
 GroupID,
 ID,
 Flags,
 Priority,
 NumKeywords
};

const char *Keywords[] =
{
 "Name=",
 "Version=",
 "BaseName=",
 "Pattern=",
 "Mask=",
 "GroupID=",
 "ID=",
 "Flags=",
 "Priority="
};

const int KeywordLength[] =
{
 5,
 8,
 9,
 8,
 5,
 8,
 3,
 6,
 9
};

/*
 *  Jetzt wirds haarig, ein Array von Funktionspointern.
 */
int (*KeywordHandler[])(struct DTDesc *) =
{
 HandleName,
 HandleVersion,
 HandleBaseName,
 HandlePattern,
 HandleMask,
 HandleGroupID,
 HandleID,
 HandleFlags,
 HandlePriority
};


