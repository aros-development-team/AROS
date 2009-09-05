#! /bin/sh
#
# Copyright (C) 2009  Free Software Foundation, Inc.
#
# This script is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Read source code from stdin and detect command names.

module=$1

grep -v "^#" | sed -n \
 -e "/grub_parser_register *( *\"/{s/.*( *\"\([^\"]*\)\".*/parser.\1: $module/;p;}" \
 -e "/grub_reader_register *( *\"/{s/.*( *\"\([^\"]*\)\".*/reader.\1: $module/;p;}" \
 -e "/grub_term_register_input *( *\"/{s/.*( *\"\([^\"]*\)\".*/terminal_input.\1: $module/;p;}" \
 -e "/grub_term_register_output *( *\"/{s/.*( *\"\([^\"]*\)\".*/terminal_output.\1: $module/;p;}" \
 -e "/grub_menu_viewer_register *( *\"/{s/.*( *\"\([^\"]*\)\".*/menu_viewer.\1: $module/;p;}"
