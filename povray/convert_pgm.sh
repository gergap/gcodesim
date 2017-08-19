#!/bin/bash

for file in workpart*.pgm; do
    [ "$file" = "workpart*.pgm" ] && exit 0
    echo "Converting $file..."
    # convert to png
    convert $file ${file%.pgm}.png || exit 1
    # remove pm on success
    rm $file
done


