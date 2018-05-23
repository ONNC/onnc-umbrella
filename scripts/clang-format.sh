#!/bin/bash -e

dir=`dirname $0`
cd $dir/..

if git show HEAD^2 >& /dev/null
then 
    echo "git diff does not work with git merge"
    echo "clang-format abort"
    exit
fi

git diff --exit-code
git diff -U0 HEAD~$@ | clang-format-diff.py -p1 -i
git diff -U0 HEAD~$@ | clang-tidy-diff.py -path build -p1 -fix
git diff --exit-code
