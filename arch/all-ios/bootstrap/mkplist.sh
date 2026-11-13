#!/bin/sh

if [ "$AROS_APPLICATION_ID" == "" ]; then
    AROS_APPLICATION_ID="org.aros.aros"
fi

sed "s/@aros_application_id@/$AROS_APPLICATION_ID/" ${1-.}/Info.plist
