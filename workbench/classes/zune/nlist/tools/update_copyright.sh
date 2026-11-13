#!/bin/sh
#
# simply bumps the year in all copyright and header files to the latest one
#
# WARNING: This script will run over ALL files regardless if they are text
# or binary files and search for the copyright notice and replace it
# immediately. So you need to be aware of that
#

YEAR=`date +%Y`

# walk through the whole directory this script is called in and search
# for files which we will try to update the Copyright notice
find . \( -not -path "*/.svn/*" -not -name "update_copyright.sh" \) -type f \
       -exec sed -i "s/Copyright.*(C).*2001-.*NList Open Source Team/Copyright (C) 2001-${YEAR} NList Open Source Team/g" {} \;
