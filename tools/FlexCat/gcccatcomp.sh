#!/bin/sh

# This script is used by gcc, and does't have to be used by anything else,
# since it relies on gcc setting some things up for it.

# $1 = .cd file
# $2 = directory where to find FlexCat and gcccatcomp.sd. 
# $3 = ,c file
# $4 = .o file


$2/FlexCat $1 $3=$2/gcccatcomp.sd && $COLLECT_GCC $3 -c -o $4
