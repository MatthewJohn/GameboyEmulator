#!/bin/bash

set -e
set -x

echo Skipped!
exit 0

bash ./tests/system/run_test.sh 02-interrupts.gb 40000000
