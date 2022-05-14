#!/bin/bash

set -e
set -x

rom=$1
timing=$2

# Start display
Xvfb :99 -screen 0 600x600x32 +extension GLX +render -noreset &
xvfb_pid=$!

# Wait for screen to start
sleep 3

export DISPLAY=:99

./GameboyEmulator -f ./tests/system/roms/$rom -b ./copyright/DMG_ROM.bin -s ./output.bmp -t $timing

# Compare image
compare -verbose -metric mae ./tests/system/expected_output/$(echo $rom | sed -E 's/\.gb$/.bmp/g') ./output.bmp -compose Src ./comparison.jpg

comparison_failed=$?

if [ "$comparison_failed" != "0" ]
then
    echo Image comparison failed!
    exit 1
fi

kill -9 $xvfb_pid


