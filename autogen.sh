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

