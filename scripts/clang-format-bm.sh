#!/bin/bash -e
dir=`dirname $0`
cd $dir/..

function format {
    find $1 -regex '.*\.\(cpp\|hpp\|cc\|cxx\)' -exec clang-format -i {} \;
    run-clang-tidy.py $1 -p build -header-filter="$1/.*" -fix
}
set -x
git diff --exit-code #you should commit before run this script
format src/tools/calibration
git diff --exit-code #return error if file changed


# how to ignore checking with clang-format
# // clang-format off
# your code
# // clang-format on 

# how to ignore checking with clang-tidy
# your code // NOLINT
