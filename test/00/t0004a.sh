#!/bin/sh
#
# test for FHS compliance
# 

tmp=/tmp/$$
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

# Test for documentation in /usr/share

for i in $here/debian/tmp/usr/share/man/man1/mawk.1.gz $here/debian/tmp/usr/share/doc/mawk/changelog.gz; do
  test -f $i
  if [ $? -ne 0 ]; then fail; fi;
done

# Test for code to create/remove /usr/doc symlink

grep -q "ln -sf ../share/doc/mawk /usr/doc/mawk" debian/tmp/DEBIAN/postinst
if [ $? -ne 0 ]; then fail; fi;

grep -q "rm -f /usr/doc/mawk" debian/tmp/DEBIAN/prerm
if [ $? -ne 0 ]; then fail; fi;

pass
