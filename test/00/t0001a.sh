#!/bin/sh
#
# test mawk binary
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

# Create temporary directory

mkdir $tmp
if [ $? -ne 0 ]; then fail; fi

# Expected Output from mawk_test

cat > $tmp/mawk_test.ok <<EOF
cd test ; ./mawktest 
mawk 1.3.3 Nov 1996, Copyright (C) Michael D. Brennan

compiled limits:
max NF             32767
sprintf buffer      1020

testing input and field splitting
input and field splitting OK

testing regular expression matching
regular expression matching OK

testing checking for write errors
checking for write errors OK

testing arrays and flow of control
array test OK

testing function calls and general stress test
general stress test passed

tested mawk seems OK
EOF
if [ $? -ne 0 ]; then fail; fi

# Run mawk_test

make mawk_test > $tmp/mawk_test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
diff -u $tmp/mawk_test.ok $tmp/mawk_test.out
if [ $? -ne 0 ]; then fail; fi

# Expected Output from fpe_test

cat > $tmp/fpe_test.ok <<EOF

testing floating point exception handling
cd test ; ./fpe_test
testing division by zero
mawk BEGIN{ print 4/0 }
inf

testing overflow
mawk BEGIN { 
  x = 100
  do { y = x ; x *= 1000 } while ( y != x )
  print "loop terminated"
}
loop terminated

testing domain error
mawk BEGIN{ print log(-8) }
nan


==============================
return1 = 0
return2 = 0
return3 = 0
results consistent: ignoring floating exceptions
EOF
if [ $? -ne 0 ]; then fail; fi

# Run fpe_test

make fpe_test > $tmp/fpe_test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
diff -u $tmp/fpe_test.ok $tmp/fpe_test.out
if [ $? -ne 0 ]; then fail; fi


pass
