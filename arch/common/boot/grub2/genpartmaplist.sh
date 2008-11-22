#! /bin/sh
#
# Copyright (C) 2005, 2008  Free Software Foundation, Inc.
#
# This script is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Read source code from stdin and detect partmap names.

module=$1

# Ignore kernel.mod.
if test $module = kernel; then
    exit
fi

# For now, this emits only a module name, if the module registers a partition map.
if grep -v "^#" | grep '^ *grub_partition_map_register' >/dev/null 2>&1; then
    echo $module
fi
