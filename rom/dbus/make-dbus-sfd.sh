#!/bin/sh

if [ $# -eq 0 ]; then
  echo "Usage: $0 <.X files>"
  echo ".X files can be created by adding \"-aux-info \`basename $@ .lo\`.X\" to Makefile"
  exit 10
fi

echo '==id $Id$'
echo '==base _DBUSBase'
echo '==basetype struct Library *'
echo '==libname dbus.library'
echo '==include <dbus/dbus.h>'
echo '==bias 96'
echo '==public'
./make-dbus-proto.sh $* | sed 's/\.\.\. *); *$/\.\.\.) (sysv)/' | sed 's/; *$/ (autoreg)/'
