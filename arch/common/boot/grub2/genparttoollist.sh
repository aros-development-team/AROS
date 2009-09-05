#! /bin/sh
#
# Copyright (C) 2009  Free Software Foundation, Inc.
#
# This gensymlist.sh is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Read source code from stdin and detect parttool names.

module=$1

grep -v "^#" | sed -n \
 -e "/grub_parttool_register *( *\"/{s/.*( *\"\([^\"]*\)\".*/\1: $module/;p;}"
