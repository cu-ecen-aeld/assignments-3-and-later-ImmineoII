#!/bin/sh

if [ ! $# -eq 2 ]; then
    echo "Invalid args"
    echo "Usage: finder.sh [target directory] [search string]"
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]; then
    echo "Target directory not a directory!"
    echo "Usage: finder.sh [target directory] [search string]"
    exit 1
fi

line_found=$( grep -r $searchstr $filesdir | wc -l)
file_found=$( grep -r $searchstr $filesdir | uniq | wc -l)
echo "The number of files are ${file_found} and the number of matching lines are ${line_found}"
