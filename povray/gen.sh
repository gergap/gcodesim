#!/bin/bash

RES="+W640 +H480"
#RES="+W320 +H200"

# remove old frames
rm -f frame*.png

./convert_pgm.sh

for file in `ls pos*.inc`; do
    num=`echo $file | sed 's/pos\([0-9]\+\).inc/\1/'`
    cat render.pov | sed "s/pos.inc/$file/" > render2.pov
    echo "Rendering frame$num.pmg ..."
    povray -D $RES +A +Oframe$num.png render2.pov 2>/dev/null || exit 1
    #exit 0
done

echo "Generating video..."
./make_video.sh

