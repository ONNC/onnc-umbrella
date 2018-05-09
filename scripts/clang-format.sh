#!/bin/bash -e

dir=`dirname $0`
cd $dir/..

git diff --exit-code
git diff -U0 HEAD^ | clang-format-diff.py -p1 -i
git diff -U0 HEAD^ | clang-tidy-diff.py -path build -p1 -fix
git diff --exit-code
