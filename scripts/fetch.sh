#!/usr/bin/env bash

# Copyright © 2004, The AROS Development Team. All rights reserved.
# $Id$


usage()
{
    error "Usage: $1 -a archive [-as archive_suffixes] [-ao archive_origins...] [-d destination] [-po patches_origins...] [-p patch[:subdir][:patch options]...]"
}

fetch()
{
    local origin="$1" file="$2" destination="$3"
    
    if echo $origin | grep ":" >/dev/null; then
        local protocol=`echo $origin | cut -d':' -f1`
    fi

    local ret=true
    
    trap 'rm -f "$destination/$file".tmp; exit' SIGINT SIGKILL SIGTERM

    case $protocol in
        http | ftp)    
            if ! wget -c "$origin/$file" -O "$destination/$file".tmp; then
                ret=false
	    else
	        mv "$destination/$file".tmp "$destination/$file"
	    fi
            rm -f "$destination/$file".tmp
	    ;;
	"")
	    if ! cp "$origin/$file" "$destination/$file"; then
	        ret=false;
	    fi
	    ;;
	*)
	    echo "Unknown protocol type \`$protocol'"
	    ret=false;;
    esac
    
    trap SIGINT SIGKILL SIGTERM
    
    $ret
}

fetch_multiple()
{
    local origins="$1" file="$2" destination="$3"
    
    local ret=false
    
    for origin in $origins; do
        echo "Trying $origin/$file..."
        if fetch "$origin" "$file" "$destination"; then
	    ret=true
	    break
	fi
    done
	
    $ret
}

fetch_cached()
{
    local origins="$1" file="$2" destination="$3"
    
    echo -n "Checking whether \`$file' is already in \`$destination'..."
    
    if ! test -e "$destination/$file"; then
        echo " NO."
	if ! test -e "$destination"; then
	    echo "\`$destination' does not exist yet. Making it."
	    mkdir -p "$destination"
	fi
	echo "Fetching \`$file'."
        fetch_multiple "$origins" "$file" "$destination"
    else
        echo " YES."
        true
    fi
}

error()
{
    echo $1
    exit 1
}


unpack()
{
    local location="$1" archive="$2";
    
    local old_PWD="$PWD"
    cd $location
    
    echo "Unpacking \`$archive'..."
    
    local ret=true
    case "$archive" in
        *.tar.bz2)
	    if ! tar xfj "$archive"; then ret=false; fi
	    ;;
        *.tar.gz | *.tgz)
	    if ! tar xfz "$archive"; then ret=false; fi
	    ;;
	*)
	    echo "Unknown archive format for \`$archive'."
	    ret=false
	    ;;
    esac
	
    cd "$old_PWD"
    
    $ret
}

unpack_cached()
{
    local location="$1" archive="$2";

    if ! test -f ${location}/.${archive}.unpacked;  then
        if unpack "$location" "$archive"; then
	    echo yes > ${location}/.${archive}.unpacked
	    true
	else
	    false
	fi
    fi
}

do_patch()
{
    local location="$1" patch_spec="$2";
    
    local old_PWD="$PWD"
    cd $location
    local abs_location="$PWD"
    
    local patch=`echo "$patch_spec": | cut -d: -f1`
    local subdir=`echo "$patch_spec": | cut -d: -f2`
    local patch_opt=`echo "$patch_spec": | cut -d: -f3`
    
    cd ${subdir:-.}
    
    local ret=true
    
    if ! patch $patch_opt < $abs_location/$patch; then
        ret=false
    fi
    
    cd "$old_PWD"
    
    $ret
}

patch_cached()
{
    local location="$1" patch="$2";
    
    local patchname=`echo $patch | cut -d: -f1`
    
    if test "x$patchname" != "x"; then
        if ! test -f ${location}/.${patchname}.applied;  then
            if do_patch "$location" "$patch"; then
	        echo yes > ${location}/.${patchname}.applied
	        true
	    else
	        false
	    fi
        fi
    fi
}


while  test "x$1" != "x"; do
    if test "x${1:0:1}" == "x-" -a  "x${2:0:1}" == "x-"; then
        usage "$0"
    fi
    
    case "$1" in
        -ao) archive_origins="$2";;
	 -a) archive="$2";;
	-as) archive_suffixes="$2";;
	 -d) destination="$2";;
	-po) patches_origins="$2";;
	 -p) patches="$2";;
  	  *) echo "Unknown option \`$1'."; usage "$0";;
    esac
    
    shift
    shift
done

test -z "$archive" && usage "$0"

archive_origins=${archive_origins:-.}
destination=${destination:-.}
patches_origins=${patches_origins:-.}

if test -n "$archive_suffixes"; then        
    archive1="$archive"
    archive=
    
    for sfx in $archive_suffixes; do
        archive2=${archive1}.$sfx
    
        fetch_cached "$archive_origins" "$archive2" "$destination" && archive="$archive2" && break
    done
else
    ! fetch_cached "$archive_origins" "$archive" "$destination" && archive=
fi

test -z "$archive" && error "Error while fetching the archive \`$archive'."

for patch in $patches; do
    patch=`echo $patch | cut -d: -f1`
    if test "x$patch" != "x"; then
        if ! fetch_cached "$patches_origins" "$patch" "$destination"; then
            error "Error while fetching the patch \`$patch'."
        fi
    fi
done

if ! unpack_cached "$destination" "$archive"; then
    error "Error while unpacking \`$archive'."
fi
    
for patch in $patches; do
    if ! patch_cached "$destination" "$patch"; then
        error "Error while applying the patch \`$patch'."
    fi
done
