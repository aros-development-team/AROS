#!/bin/sh
# Short script to check for used tools and scripts
# and generate them if necessary
#
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

echo "Running autoconf..."
autoconf

echo "Checking for AROS Tools..."

cd tools

echo -n "Checking for Makefile Generation Tool... "
if [ -x "genmf/genmf" ] ; then
  echo "tools/genmf/genmf"
else
  echo "Not found!"
  echo "Trying to generate it..."
  cd genmf
  make
  if [ -x "genmf" ] ; then
    echo "Successfully generated tool."
  else
    echo "Failed to generate tools/genmf/genmf !!!"
    exit 10
  fi
  cd ..
fi

echo -n "Checking for C-Source Packer Tool... "
if [ -x "cpak/cpak" ] ; then
  echo "tools/cpak/cpak"
else
  echo "Not found!"
  echo "Trying to generate it..."
  cd cpak
  make
  if [ -x "cpak" ] ; then
    echo "Successfully generated tool."
  else
    echo "Failed to generate tools/cpak/cpak !!!"
    exit 10
  fi
  cd ..
fi

echo -n "Checking for Source Archive Tool... "
if [ -x "archtools/archtool" ] ; then
  echo "tools/archtools/archtool"
else
  echo "Not found!"
  echo "Trying to generate it..."
  cd archtools
  make
  if [ -x "archtool" ] ; then
    echo "Successfully generated tool."
  else
    echo "Failed to generate tools/archtools/archtool !!!"
    exit 10
  fi
  cd ..
fi

echo -n "Checking for MetaMake... "
if [ -x "`which mmake`" ] ; then
  MMAKE="mmake"
  echo `which mmake`
else
  if [ -x "MetaMake/mmake" ] ; then
    echo "locally installed in tools/MetaMake/mmake."
  else
    echo "Not found!"
    echo "Trying to generate it..."
    cd MetaMake
    ./configure
    make
    if [ -x "mmake" ] ; then
      echo "Successfully generated tool."
    else
      echo "Failed to generate tools/MetaMake/mmake !!!"
      exit 10
    fi
    cd ..
  fi
  MMAKE="tools/MetaMake/mmake"
fi

cd ..
echo -n "Checking for MetaMake Config File..."
if [ -f "$HOME/.mmake.config" ] ; then
  echo "$HOME/.mmake.config"
else
  if [ -f "mmake.config" ] ; then
    echo "locally installed in mmake.config."
  else
    echo "Not found!"
    echo "Using default."
    cp mmake.config.in mmake.config.tmp
    echo -n "Do you want to edit it now [y/N]?"
    read edit
    case "$edit" in
      [yY] | [yY]es )
	if [ ! -x "$EDITOR" ] ; then
	  echo -n "What is your favourite editor?"
	  read EDITOR
	fi
	$EDITOR mmake.config.tmp
	echo -n "Do you want to copy it into your Home [y/N]?"
	read tohome
	case "$tohome" in
	  [yY] | [yY]es )
	    mv mmake.config.tmp $HOME/.mmake.config
	    ;;
	  *)
	    mv mmake.config.tmp mmake.config
	    ;;
	esac
	;;
      *)
	mv mmake.config.tmp mmake.config
	;;
    esac
  fi
fi

cd $ORIGDIR

echo "Running $srcdir/configure..."
$srcdir/configure "$@"

echo
echo "You have installed all necessary tools to compile AROS now."
echo "$srcdir/configure has been run with your command line options."
echo
echo "Now type 'make' to start building the tree."
echo

exit 0

