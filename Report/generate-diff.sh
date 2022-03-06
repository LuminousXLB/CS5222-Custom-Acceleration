#!/bin/bash

SRC=mmult_float.cpp
SRC_DIR=../history

DST_DIR=program

DIFF="diff -u"

BASE1B=$SRC_DIR/1a-baseline-autopipe/$SRC
# 1a-baseline-nopipe

$DIFF $BASE1B $SRC_DIR/1b1-pipeline-L3/$SRC >$DST_DIR/1b1-pipeline-L3.diff
$DIFF $BASE1B $SRC_DIR/1b2-pipeline-L2-1WnR/$SRC >$DST_DIR/1b2-pipeline-L2-1WnR.diff
$DIFF $BASE1B $SRC_DIR/1b2-pipeline-L2-T2P/$SRC >$DST_DIR/1b2-pipeline-L2-T2P.diff
$DIFF $BASE1B $SRC_DIR/1b3-pipeline-L1-1WnR/$SRC >$DST_DIR/1b3-pipeline-L1-1WnR.diff
$DIFF $BASE1B $SRC_DIR/1b3-pipeline-L1-T2P/$SRC >$DST_DIR/1b3-pipeline-L1-T2P.diff


BASE1C=$SRC_DIR/1c0-baseline-ap-l2/$SRC

$DIFF $BASE1C $SRC_DIR/1c1-partition-ap-d1-f2/$SRC >$DST_DIR/1c1-partition-ap-d1-f2.diff
$DIFF $BASE1C $SRC_DIR/1c1-partition-ap-d2-f2/$SRC >$DST_DIR/1c1-partition-ap-d2-f2.diff




# 1c2-partition-ap-d2-f16
# 1c2-partition-ap-d2-f32
# 1c2-partition-ap-d2-f4
# 1c2-partition-ap-d2-f8
