#!/bin/sh

counter=`ls -al counter.txt | cut -c30-41`
printf "Content-type: text/html\n\n%06d\n" "$counter"
echo >> counter.txt
