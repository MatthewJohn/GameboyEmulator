#!/bin/bash
# Must display in 1-byte hex
od -An -vtx1 "$1"
