#!/bin/sh

# This script is used by gcc, and does't have to be used by anything else,
# since it relies on gcc setting some things up for it.

cd_file=$1  # $1 = .cd file
dir=$2      # $2 = directory where to find FlexCat and gcccatcomp.sd. 
c_file=$3   # $3 = ,c file

shift 3     # parameters from 4 to n are passed "as is" to $COLLECT_GCC.

$dir/FlexCat $cd_file $c_file=$dir/gcccatcomp.sd && $COLLECT_GCC $c_file $@
