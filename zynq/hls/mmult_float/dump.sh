#!/bin/bash

set -x
set -e

vitis_hls -f hls_nopipe.tcl

DIR=archive/$(date +%Y%m%d-%H%M%S)$1
mkdir $DIR
mv accel/solution0/solution0.log $DIR
cp accel/solution0/syn/report/mmult_hw_csynth.rpt $DIR