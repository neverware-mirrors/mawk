#!/bin/sh
#
# test for update-alternatives doing the Right thing with manpages
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

postinst=$here/debian/tmp/DEBIAN/postinst

# Check for pre-FHS manpage arguments

grep -q -- "--slave /usr/man/" $postinst
if [ $? -ne 1 ]; then fail; fi;

# Check for FHS manpage arguments

grep -q -- "--slave /usr/share/man/" $postinst
if [ $? -ne 0 ]; then fail; fi;

pass
