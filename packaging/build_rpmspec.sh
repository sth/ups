#!/bin/sh

# -----------------------------------------------------------------------------
#  Write an RPM specification file
# -----------------------------------------------------------------------------

echo
if [ -z "${RELTAG}" ]
then
    echo "RELTAG not set"
    exit 1
fi

if [ -d /usr/src/packages ]
then
    pkg=/usr/src/packages
    echo "Found packages directory $pkg"
elif [ -d /usr/src/redhat ]
then
    pkg=/usr/src/redhat
    echo "Found RedHat packages directory $pkg"
else
    pkg=/usr/src/packages
    echo "Assuming packages directory is $pkg"
fi

release=`date '+%Y%m%d'`
version=`expr ${RELTAG} : '\([0-9]\.[0-9]*\)'`
file=ups-$version.spec
echo "Writing RPM spec file $file"
sed \
	-e "s/__RELTAG__/${RELTAG}/" \
	-e "s/__RELEASE__/$release/" \
	-e "s/__VERSION__/$version/" \
	< ups-rpm-spec > $file

echo
echo "To build this do -"
echo
echo "  cd `pwd`"
echo "  cp ../releases/ups-${RELTAG}.tar.gz $pkg/SOURCES"
echo "  cp ups.wmconfig ups.sh  $pkg/SOURCES"
echo "  cp $file  $pkg/SPECS"
echo "  cd $pkg/SPECS"
echo "  rpm -ba $file"
echo

exit 0

