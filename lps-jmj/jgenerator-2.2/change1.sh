#!/bin/sh

sed -e 's/com.iv/org.openlaszlo.iv/g' < $1 > /tmp/foo
mv /tmp/foo $1
