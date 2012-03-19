#!/bin/sh
#
# get svn revision via svn or via git
#

if test -d $1/.svn; then
    svn info $(SRCDIR) | sed -n 's/Revision: //p'
else
    if test -d $1/.git; then
        cd $1
        git log -1 | grep git-svn-id | sed 's|.*@\(.*\) .*|\1|'
    else
        echo "NoRev"
    fi
fi
