#!/bin/sh
#
# get repository revision via svn or via git
#

inside_git_repo=`cd $1 && git rev-parse --is-inside-work-tree 2>/dev/null`

if test -d $1/.svn; then
    svn info $1 | sed -n 's/Revision: //p'
else
    if [ "$inside_git_repo" = "true" ]; then
        cd $1
        git rev-parse --short HEAD
    else
        echo "NoRev"
    fi
fi
