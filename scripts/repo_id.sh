#!/bin/sh
#
# get repository ID via svn or via git
#

inside_git_repo=`cd $1 && git rev-parse --is-inside-work-tree 2>/dev/null`

if test -d $1/.svn; then
    cd $1
    svn info | grep '^URL' | awk '{print $NF}'
else
    if [ "$inside_git_repo" = "true" ]; then
        cd $1
        git config --get remote.origin.url
    else
        echo "Unknown"
    fi
fi
