#!/bin/sh
#
# test for common copyright file errors
# 

here=$(pwd)

if [ $? -ne 0 ]; then exit 1; fi

fail()
{

    echo FAILED 1>&2
    cd $here
    rm -fr $tmp
    exit 1

}

pass()
{

    cd $here
    rm -fr $tmp
    exit 0

}

trap "fail" 1 2 3 15

copyright=$here/debian/tmp/usr/share/doc/mawk/copyright

# "Debian GNU/Linux" upsets Hurd people

grep -qi linux $copyright
if [ $? -ne 0 ]; then fail; fi

# We are not the hello source package

grep -qi "hello source package" $copyright
if [ $? -ne 1 ]; then fail; fi

# Check for old place of GPL

grep -q "/usr/doc/copyright/GPL" $copyright
if [ $? -ne 1 ]; then fail; fi;

# Check for new, correct place of GPL

grep -q "/usr/share/common-licenses/GPL" $copyright
if [ $? -ne 0 ]; then fail; fi;

pass
