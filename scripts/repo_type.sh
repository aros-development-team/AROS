#!/bin/sh
#
# get repository type
#

inside_git_repo=`cd $1 && git rev-parse --is-inside-work-tree 2>/dev/null`

if test -d $1/.svn; then
    echo "SVN"
else
    if [ "$inside_git_repo" = "true" ]; then
        echo "Git"
    else
        echo ""
    fi
fi
