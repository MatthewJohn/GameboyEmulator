#!/bin/bash

set -e
set -x

bash ./tests/system/run_test.sh 06-ld_r,r.gb 30000000
