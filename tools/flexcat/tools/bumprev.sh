#!/bin/bash
#
# a simple shell script which allows to bump the EXE_REVISION and EXE_DATE information
# found in a specified source code file
#

if [ "$1" = "all" ]; then
   find . -maxdepth 3 -name "version.h" -exec $0 {} \;
else
   if [ -e $1 ]; then
      foundrev=`grep "#define EXE_REVISION   " $1`
      if [ $? -eq 0 ]; then
         founddate=`grep "#define EXE_DATE " $1`
         if [ $? -eq 0 ]; then
            newrev=`echo $foundrev | awk '{ print $1" "$2"   "$3+1 }'`
            curdate=`date +%d.%m.%Y`
            newdate=`echo "#define EXE_DATE       \"${curdate}\""`
            sed -i "s/${foundrev}/${newrev}/g" $1
            sed -i "s/${founddate}/${newdate}/g" $1
            echo "bumped $1"
         fi
      fi
   fi
fi
