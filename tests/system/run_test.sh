#!/bin/bash

rom=$1
timing=$2
echo $timing
./GameboyEmulator -f ./tests/system/roms/$rom -b ./copyright/DMG_ROM.bin -s ./output.bmp -t $timing

