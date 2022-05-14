#!/bin/bash

set -e
set -x

rom=$1
timing=$2

# Start display
Xvfb :99 -screen 0 600x400x16 +extension GLX +render -noreset &
xvfb_pid=$!

# Wait for screen to start
sleep 3

export DISPLAY=:99

./GameboyEmulator -f ./tests/system/roms/$rom -b ./copyright/DMG_ROM.bin -s ./output.bmp -t $timing

# Compare image
compare -verbose -metric mae ./output.bmp -compose Src ./tests/system/expected_output/$(echo $rom | sed 's/\.gb$/.bmp/g')

comparison_failed=$?

if [ "$comparison_failed" != "0" ]
then
    echo Image comparison failed!
    exit 1
fi

kill -9 $xvfb_pid


