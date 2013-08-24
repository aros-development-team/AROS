#!/bin/sh

# Please extend this script, it should generate everything that is possible from the SFD file

SFD=../developer/sfd/codesets_lib.sfd

sfdc $SFD --mode=macros --target=i386-aros --output=defines/codesets.h
