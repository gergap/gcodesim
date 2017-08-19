#!/bin/bash
INPUT=frame%04d.png
OUTPUT=demo.mp4
FRAMERATE=30

ffmpeg -r $FRAMERATE -f image2 -i $INPUT -vcodec libx264 -crf 25 $OUTPUT

