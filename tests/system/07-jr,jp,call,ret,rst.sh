#!/bin/bash

set -e
set -x

bash ./tests/system/run_test.sh 07-jr,jp,call,ret,rst.gb 35000000
