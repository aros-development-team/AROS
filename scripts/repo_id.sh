#!/bin/sh
#
# get repository ID via svn or via git
#

inside_git_repo="$(git rev-parse --is-inside-work-tree 2>/dev/null)"

if test -d $1/.svn; then
    cd $1
    svn info | grep '^URL' | awk '{print $NF}'
else
    cd $1
    if [ "$inside_git_repo" == "true" ]; then
        git config --get remote.origin.url
    else
        echo "Unknown"
    fi
fi
