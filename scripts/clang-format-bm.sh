#!/bin/bash -e
dir=`dirname $0`
cd $dir/..
root=$PWD

function format {
    run-clang-tidy.py $1 -p build -header-filter="$1/.*" -fix -quiet -j4
    find $1 -regex '.*\.\(cpp\|h\|hpp\|cc\|cxx\)' -exec clang-format -i {} \;
}
set -x
git diff --exit-code #you should commit before run this script
format $PWD/src/tools/calibration
format $PWD/src/lib/Target/TG
git diff --exit-code #return error if file changed


# how to ignore checking with clang-format
# // clang-format off
# your code
# // clang-format on 

# how to ignore checking with clang-tidy
# your code // NOLINT
