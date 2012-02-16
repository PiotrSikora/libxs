#!/bin/sh
#
# This script extracts the Crossroads version from include/xs.h,
# which is the master location for this information.
#
if [ ! -f include/xs.h ]; then
    echo "version.sh: error: include/xs.h does not exist" 1>&2
    exit 1
fi
MAJOR=`egrep '^#define +XS_VERSION_MAJOR +[0-9]+$' include/xs.h`
MINOR=`egrep '^#define +XS_VERSION_MINOR +[0-9]+$' include/xs.h`
PATCH=`egrep '^#define +XS_VERSION_PATCH +[0-9]+$' include/xs.h`
if [ -z "$MAJOR" -o -z "$MINOR" -o -z "$PATCH" ]; then
    echo "version.sh: error: could not extract version from include/xs.h" 1>&2
    exit 1
fi
MAJOR=`echo $MAJOR | awk '{ print $3 }'`
MINOR=`echo $MINOR | awk '{ print $3 }'`
PATCH=`echo $PATCH | awk '{ print $3 }'`
echo $MAJOR.$MINOR.$PATCH | tr -d '\n'

