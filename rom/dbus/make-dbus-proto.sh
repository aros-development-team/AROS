#!/bin/sh

if [ $# -eq 0 ]; then
  echo "Usage: $0 <.X files>"
  echo ".X files can be created by adding \"-aux-info \`basename $@ .lo\`.X\" to Makefile"
  exit 10
fi

grep -h '\.c' $* | cpp | grep -v '^ *static' | grep -v '^#' | grep -v '[ *]_dbus_' | sed 's/^ *extern //' 
