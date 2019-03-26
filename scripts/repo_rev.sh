#!/bin/sh
#
# get svn revision via svn or via git
#

inside_git_repo="$(git rev-parse --is-inside-work-tree 2>/dev/null)"

if test -d $1/.svn; then
    svn info $1 | sed -n 's/Revision: //p'
else
    cd $1
    if [ "$inside_git_repo" == "true" ]; then
        git log -1 | grep git-svn-id | sed 's|.*@\(.*\) .*|\1|'
    else
        echo "NoRev"
    fi
fi
