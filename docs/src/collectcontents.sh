#!/bin/sh

TOP="$1"

gawk -f contents2html.gawk --assign TOP="$TOP" `find $TOP -name contents`
