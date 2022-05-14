#!/bin/bash

set -e
set -x

bash ./tests/system/run_test.sh 04-op_r,imm.gb 40000000
