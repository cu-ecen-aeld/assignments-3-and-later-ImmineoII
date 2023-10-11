#!/bin/sh

if [ ! $# -eq 2 ]; then
    echo "Invalid args"
    echo "Usage: writer.sh [target file] [string]"
    exit 1
fi

writefile=$1
writestr=$2

if [ -d $writefile ]; then
    echo "Target file is a directory!"
    echo "Usage: writer.sh [target file] [string]"
    exit 1
fi

target_directory=$(dirname $writefile)

if [ ! -d $target_directory ];then
    mkdir -p $target_directory
fi

echo $writestr > $writefile
