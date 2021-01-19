#!/usr/bin/env bash

# Copyright © 2004-2021, The AROS Development Team. All rights reserved.
# $Id$

error()
{    
    echo "$1"
    exit 1
}

usage()
{
    error "Usage: $1 -a archive [-s suffixes] [-ao archive_origins...] [-l location to download to] [-d dir to unpack to] [-po patches_origins...] [-p patch[:subdir][:patch options]...]"
}

fetch_mirrored()
{
    local origin="$1" file="$2" destination="$3" mirrosgroup="$4" mirrors="$5"
    local ret=false

    for mirror in $mirrors; do
        echo "Downloading from ${mirrosgroup}... "
        if fetch "${mirror}/$origin" "${file}" "$destination"; then
                ret=true
                break;
        fi
    done
    
    $ret
}

gnu_mirrors="http://mirror.netcologne.de/gnu http://ftp.gnu.org/pub/gnu ftp://ftp.heanet.ie/pub/gnu ftp://mirror.jre655.com/GNU"

fetch_gnu()
{
    local origin="$1" file="$2" destination="$3"
    local ret=true

    if ! fetch_mirrored "$origin" "${file}" "$destination" "GNU" "${gnu_mirrors}"; then
        ret=false
    fi

    $ret
}

# Note: at time of writing, the main SF download server redirects to mirrors
# and corrects moved paths, so we retry this server multiple times (in one
# wget call).
sf_mirrors="http://downloads.sourceforge.net"

fetch_sf()
{
    local origin="$1" file="$2" destination="$3"
    local ret=true

    if ! fetch_mirrored "$origin" "${file}" "$destination" "SourceForge" "${sf_mirrors}"; then
        ret=false
    fi

    $ret
}

github_mirrors="https://github.com"

fetch_github()
{
    local origin="$1" file="$2" destination="$3"
    local ret=true

    if ! fetch_mirrored "$origin" "$file" "$destination" "Github" "${github_mirrors}"; then
        ret=false
    fi

    $ret
}

wget_ftp()
{
    local wgetsrc="$1" wgetoutput="$2"
    local wgetextraflags
    local ret=true

    for (( ; ; ))
    do
        if !  eval "wget -t 3 --retry-connrefused $wgetextraflags -T 15 -c $wgetsrc -O $wgetoutput"; then
            if test "$ret" = false; then
                break
            fi
            ret=false
            wgetextraflags="--secure-protocol=TLSv1"
        else
            ret=true
            break
        fi
    done

    $ret
}

wget_http()
{
    local tryurl="$1" wgetoutput="$2"
    local wgetextraflags=""
    local wgetext=""
    local wgetsrc
    local urlsrc
    local ret

    if echo "$tryurl" | grep "prdownloads.sourceforge.net" >/dev/null; then
        if ! echo "$tryurl" | grep "?download" >/dev/null; then
            wgetext="?download"
        fi
    fi

    urlsrc=$(wget --no-verbose --method=HEAD --output-file - "$tryurl$wgetext")
    ret=$?
    if [ $ret -ne 0 ]; then
        urlsrc=$(wget --secure-protocol=TLSv1 --no-verbose --method=HEAD --output-file - "$tryurl$wgetext")
        ret=$?
    fi
    if [ $ret -ne 0 ]; then
        wgetsrc="$tryurl"
    else
        local origIFS=$IFS
        IFS=$'\n' wgetsrc=($(echo "$urlsrc" | cut -d' ' -f4))
        IFS=$origIFS
    fi
    ret=true

    for (( ; ; ))
    do
        if ! eval "wget -t 3 --retry-connrefused $wgetextraflags -T 15 -c $wgetsrc -O $wgetoutput"; then
            if test "$ret" = false; then
                break
            fi
            ret=false
            wgetextraflags="--secure-protocol=TLSv1"
        else
            ret=true
            break
        fi
    done

    $ret
}

fetch()
{
    local origin="$1" file="$2" destination="$3"
    
    local protocol
    
    if echo "$origin" | grep ":" >/dev/null; then
        protocol=$(echo "$origin" | cut -d':' -f1)
    fi

    local ret=true
    
    trap 'rm -f "$destination/$file".tmp; exit' SIGINT SIGKILL SIGTERM

    case $protocol in
        https| http)
            if ! wget_http "$origin/$file" "$destination/$file.tmp"; then
                ret=false
            else
                mv "$destination/$file.tmp" "$destination/$file"
            fi
            rm -f "$destination/$file.tmp"
            ;;
        ftp)    
            if ! wget_ftp "$origin/$file" "$destination/$file.tmp"; then
                ret=false
            else
                mv "$destination/$file.tmp" "$destination/$file"
            fi
            rm -f "$destination/$file.tmp"
            ;;
        gnu)
            if ! fetch_gnu "${origin:${#protocol}+3}" "$file" "$destination"; then
                ret=false
            fi
            ;;
        sf | sourceforge)
            if ! fetch_sf "${origin:${#protocol}+3}" "$file" "$destination"; then
                ret=false
            fi
            ;;
        github)
            if ! fetch_github "${origin:${#protocol}+3}" "$file" "$destination"; then
                ret=false
            fi
            ;;
	*)
	    if test "$origin" = "$destination";  then
	        ! test -f "$origin/$file" && ret=false
	    else
	        if ! cp "$origin/$file" "$destination/$file.tmp" >/dev/null; then
		    ret=false
		else
		    mv "$destination/$file.tmp" "$destination/$file"
		fi
	    fi
	    ;;
    esac
    
    trap SIGINT SIGKILL SIGTERM
    
    $ret
}

fetch_multiple()
{
    local origins="$1" file="$2" destination="$3"
    for origin in $origins; do
        echo "Trying     $origin/$file..."
        fetch "$origin" "$file" "$destination" && return  0
    done

    return 1
}

fetch_cached()
{ 
    local origins="$1" file="$2" suffixes="$3" destination="$4" foundvar="$5"
    
    local __dummy__
    foundvar=${foundvar:-__dummy__}
    
    export "$foundvar"=
    
    test -e "$destination" -a ! -d "$destination" && \
        echo "fetch_cached: \`$destination' is not a diretory." && return 1

    if ! test -e "$destination"; then
	echo "fetch_cached: \`$destination' does not exist. Making it."
	! mkdir -p "$destination" && return 1
    fi
    
    if test "x$suffixes" != "x"; then
        for sfx in $suffixes; do
	    fetch_multiple "$destination" "$file.$sfx" "$destination" && \
	        export "$foundvar"="$file.$sfx" && return 0
        done
       
	for sfx in $suffixes; do
	    fetch_multiple "$origins" "$file.$sfx" "$destination" && \
	        export "$foundvar"="$file.$sfx" && return 0
        done    
    else
        fetch_multiple "$destination" "$file" "$destination" && \
	    export "$foundvar"="$file" && return 0
        fetch_multiple "$origins" "$file" "$destination" && \
	    export "$foundvar"="$file" && return 0
    fi
    
    return 1
}

unpack()
{
    local location="$1" archive="$2" archivepath="$3";
    
    local old_PWD="$PWD"
    cd "$location"
    
    echo "Unpacking  \`$archive'..."
    
    local ret=true
    case "$archive" in
        *.tar.bz2)
	    if ! tar xfj "$archivepath/$archive"; then ret=false; fi
	    ;;
        *.tar.gz | *.tgz)
	    if ! tar xfz "$archivepath/$archive"; then ret=false; fi
	    ;;
        *.zip)
	    if ! unzip "$archivepath/$archive"; then ret=false; fi
	    ;;
        *.tar.xz)
	    if ! tar xfJ "$archivepath/$archive"; then ret=false; fi
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
    local location="$1" archive="$2" archivepath="$3";

    if ! test -e "$location"; then
	echo "unpack_cached: \`$location' does not exist. Making it."
	! mkdir -p "$location" && return 1
    fi

    if ! test -f "${location}/.${archive}.unpacked";  then
        if unpack "$location" "$archive" "$archivepath"; then
	    echo yes > "${location}/.${archive}.unpacked"
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
    cd "$location"
    local abs_location="$PWD"
    
    local patch=$(echo "$patch_spec": | cut -d: -f1)
    local subdir=$(echo "$patch_spec": | cut -d: -f2)
    local patch_opt=$(echo "$patch_spec": | cut -d: -f3 | sed -e "s/,/ /g")
    local patch_cmd="patch -Z $patch_opt < $abs_location/$patch"
        
    cd "${subdir:-.}"  2> /dev/null;
    
    local ret=true

    if ! eval "$patch_cmd" ; then
        ret=false
    fi

    cd "$old_PWD"
    
    $ret
}

patch_cached()
{
    local location="$1" patch="$2";
    
    local patchname=$(echo "$patch" | cut -d: -f1)
    
    if test "x$patchname" != "x"; then
        if ! test -f "${location}/.${patchname}.applied";  then
            if do_patch "$location" "$patch"; then
	        echo yes > "${location}/.${patchname}.applied"
	        true
	    else
	        false
	    fi
        fi
    fi
}

    
fetchlock()
{
    local location="$1" archive="$2";
    local notified=0;

    for (( ; ; ))
    do
        if ! test -f "$location"; then
            if ! test -f "${location}/${archive}.fetch"; then
                trap "fetchunlock $location $archive" INT
                echo "$$"  > "${location}/${archive}.fetch"
                break
            else
                if test "x${notified}" != "x1"; then
                    echo "fetch.sh: waiting for process with PID: $(<"${location}/${archive}.fetch") to finish downloading..."
                fi
                notified=1
                continue
            fi
        else
            break
        fi
    done
}


fetchunlock()
{
    local location="$1" archive="$2";

    if test -f "${location}/${archive}.fetch"; then
        rm -f "${location}/${archive}.fetch"
    fi
}

location="./"

while  test "x$1" != "x"; do
    if test "x${1:0:1}" == "x-" -a  "x${2:0:1}" == "x-"; then
        usage "$0"
    fi
    
    case "$1" in
        -ao) archive_origins="$2";;
	 -a) archive="$2";;
	 -s) suffixes="$2";;
	 -d) destination="$2";;
	-po) patches_origins="$2";;
	 -p) patches="$2";;
     -l) location="$2";;
  	  *) echo "fetch: Unknown option \`$1'."; usage "$0";;
    esac
    
    shift
    shift
done

test -z "$archive" && usage "$0"

archive_origins=${archive_origins:-.}
destination=${destination:-.}
location=${location:-.}
patches_origins=${patches_origins:-.}

fetchlockfile="$archive"
fetchlock "$location" "$fetchlockfile"

fetch_cached "$archive_origins" "$archive" "$suffixes" "$location" archive2

test -z "$archive2" && fetchunlock "$location" "$fetchlockfile" && error "fetch: Error while fetching the archive \`$archive'."
archive="$archive2"

for patch in $patches; do
    patch=$(echo "$patch" | cut -d: -f1)
    if test "x$patch" != "x"; then
        if ! fetch_cached "$patches_origins" "$patch" "" "$destination"; then
            fetch_cached "$patches_origins" "$patch" "tar.bz2 tar.gz zip" "$destination" patch2
            test -z "$patch2" && error "fetch: Error while fetching the patch \`$patch'."
            if ! unpack_cached "$destination" "$patch2" "$destination"; then
                fetchunlock "$location" "$fetchlockfile"
                error "fetch: Error while unpacking \`$patch2'."
            fi
        fi
    fi
done

if test "x$suffixes" != "x"; then
    if ! unpack_cached "$destination" "$archive" "$location"; then
        fetchunlock "$location" "$fetchlockfile"
        error "fetch: Error while unpacking \`$archive'."
    fi
fi
    
for patch in $patches; do
    if ! patch_cached "$destination" "$patch"; then
        fetchunlock "$location" "$fetchlockfile"
        error "fetch: Error while applying the patch \`$patch'."
    fi
done

fetchunlock "$location" "$fetchlockfile"
