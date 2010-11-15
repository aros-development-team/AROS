#!/bin/sh

if [ "$1" = "" ]; then
    echo "Usage: $0 \<app_bundle_name\>"
    echo "The following environment variables must be set:"
    echo "  CODE_SIGN_PROFILE - name of provisioning profile to use, without .mobileprovision suffix."
    echo "  CODE_SIGN_ENTITY  - name of developer certificate in the keychain."
    exit 0;
fi

profile_pathname=~/Library/MobileDevice/Provisioning\ Profiles/$CODE_SIGN_PROFILE.mobileprovision

if [ ! -f "$profile_pathname" ]; then
    echo "ERROR: Provisioning profile $CODE_SIGN_PROFILE not found"
    exit 255
fi

echo Embedding provisioning profile...
cp -f "$profile_pathname" "$1/embedded.mobileprovision"

echo Creating code signature...
codesign -f -s "$CODE_SIGN_ENTITY" --resource-rules=$1/ResourceRules.plist --entitlements Entitlements.plist $1
