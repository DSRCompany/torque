#!/bin/bash

# this file merges gcov coverage markup files from test and gtest folders and calculates overall coverage using con_gcov.py script.

# ASSUMPTION: gcov files are placed int test/<module_name>/ and gtest/<module_name>/ subdirs.
#             con_gcov.py is placed in the same folder as this script

DIR=$(dirname $0)
MERGE_SCRIPT="$DIR/con_gcov.py"

for i in test/*/*.gcov
do
  if [ -f "g$i" ]
  then
    # do the job
    python $MERGE_SCRIPT -f"$i" -f"g$i"
  fi
done
