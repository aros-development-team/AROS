#!/bin/sh

echo "Content-type: text/html"
echo
counter=`cat counter.txt`
if [ -z "$counter" ]; then
	counter="1"
fi
echo "$counter" | gawk ' { printf ("%06d\n", $1); }'
counter=`expr $counter + 1`
echo $counter > counter.txt
