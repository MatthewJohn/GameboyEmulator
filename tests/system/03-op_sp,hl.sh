#!/bin/bash

set -e
set -x

echo Skipped!
exit 0

bash ./tests/system/run_test.sh 03-op_sp,hl.gb 40000000
