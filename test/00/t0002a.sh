#!/bin/sh
#
# test for lintian clean package
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

# Run lintian

lintian $here/../mawk_$(dpkg-parsechangelog | grep "^Version: " | sed -e "s/Version: \(.*\)/\1/")_$(dpkg --print-architecture).deb
if [ $? -ne 0 ]; then fail; fi

pass
