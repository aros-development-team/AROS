#!/bin/sh
#
# get repository type
#

inside_git_repo="$(git rev-parse --is-inside-work-tree 2>/dev/null)"

if test -d $1/.svn; then
    echo "SVN"
else
    cd $1
    if [ "$inside_git_repo" == "true" ]; then
        echo "Git"
    else
        echo ""
    fi
fi
