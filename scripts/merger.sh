#!/bin/sh

mydir=$1
olddir=$2
yourdir=$3

cp=cp
mkdirhier="mkdir -p"
#cp="echo cp"
#mkdirhier="echo mkdirhier"

export mydir olddir yourdir cp mkdirhier

rm -f conflicts.log added.log

# First try to merge all the changes from olddir to yourdir into mydir
# that means that only the files which are both in mydir, yourdir and olddir have to be processed

find $mydir -type f -exec sh -c                                           \
    'cur="`echo "{}" | cut -d/ -f2-`";                                    \
    if [ -e $olddir/"$cur" ] && [ -e $yourdir/"$cur" ]; then              \
        echo processing "$cur";                                           \
        merge -p $mydir/"$cur" $olddir/"$cur" $yourdir/"$cur" >merge.tmp; \
        if [ $? -gt 0 ]; then                                             \
            echo "$cur" >> conflicts.log;                                 \
        fi;                                                               \
        if ! cmp $mydir/"$cur" merge.tmp >/dev/null; then                 \
            $cp merge.tmp $mydir/"$cur";                                  \
            echo "$cur" has been updated;                                 \
        fi;                                                               \
    fi'                                                                   \
';'

# Then copy all the files which are new into mydir. A file is "new" when it and all of its ancestors are
# not present in olddir nor in mydir. If a file is present in olddir but not in mydir it means that it's
# been removed and therefore doesn't have to be copied; if it's present both in mydir and olddir it means
# that if it needed to be modified it's already been modified by the previous part, therefore doesn't need to
# be copied either. All that means that if a file is present both in yourdir and olddir it doesn't need to be
# copied

find $yourdir -type f -exec sh -c                                      \
    'cur="`echo "{}" | cut -d/ -f2-`";                                 \
    if ! [ -e $olddir/"$cur" ] && ! [ -e $mydir/"$cur" ] &&            \
    (                                                                  \
        count=1;                                                       \
        oldcur2="";                                                    \
        while true; do                                                 \
            cur2=`echo "$cur" | cut -d/ -f-$count`;                    \
            if [ "$oldcur2" == "$cur2" ]; then                         \
                exit 0;                                                \
            fi;                                                        \
            if [ -e $olddir/"$cur2" ] && ! [ -e $mydir/"$cur2" ]; then \
                exit 1;                                                \
            fi;                                                        \
            oldcur2="$cur2";                                           \
            count=$[$count + 1];                                       \
        done                                                           \
    ); then                                                            \
        echo Adding "$cur";                                            \
        dirname=`dirname "$cur"`;                                      \
        $mkdirhier $mydir/"$dirname";                                  \
        $cp $yourdir/"$cur" $mydir/"$dirname";                         \
        echo "$cur" >> added.log;                                      \
    fi'                                                                \
';'

echo Operation finished.

if [ -e conflicts.log ]; then                                                                                   \
    echo There were `wc -l conflicts.log` conflicts. Have a look at conflicts.log to see in which files.; \
else                                                                                                            \
    echo There were no conflicts;                                                                               \
fi

if [ -e added.log ]; then                                                                            \
    echo `wc -l added.log` files have been added. Have a look at added.log to see which ones.; \
else                                                                                                 \
    echo No new files have been added;                                                               \
fi

