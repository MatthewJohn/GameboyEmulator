#!/bin/bash

set -e
set -x

bash ./tests/system/run_test.sh 10-bit_ops.gb 100000000
