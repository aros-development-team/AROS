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
       -exec sed -i "s/Copyright.*(C).*2005-.*codesets.library Open Source Team/Copyright (C) 2005-${YEAR} codesets.library Open Source Team/g" {} \;
