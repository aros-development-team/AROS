#!/bin/sh

counter=`cat counter.txt`
if [ -z "$counter" ]; then
	counter="1"
fi
printf "Content-type: text/html\n\n%06d\n" "$counter"
counter=`expr $counter + 1`
echo $counter > counter.txt
