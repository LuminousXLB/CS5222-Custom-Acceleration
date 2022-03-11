#!/bin/bash

set -x
set -e

DIR=./archive/$(date +%Y%m%d-%H%M%S)$1
mkdir $DIR
cp mmult_float.cpp $DIR
cp mmult.h $DIR

vitis_hls -f hls.tcl

mv vitis_hls.log $DIR/hls.log
cp accel/solution0/syn/report/mmult_hw_csynth.rpt $DIR
cp accel/solution0/solution0_data.json $DIR
