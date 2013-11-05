#!/bin/bash

# Removes the old gcov data files (.gcda)
#lcov --directory . --zerocounters

# Run the code so the gcov data files (.gcda) are generated. For example:
#make check

# Make empty .gcda files for each existing .gcno file.
# This is so report will show 0.0% for missing tests.
# This step is not needed.
find . -name "*.gcno" | while read line
do
  gcda=`echo $line | sed 's/\.gcno$/.gcda/'`
  if [ \! -e $gcda ]
  then
    touch $gcda && echo created empty $gcda
  fi
done

# Generate the lcov trace file.
lcov --capture --directory . --output-file all.info

# Remove old generate webpages.
rm -rf coverage/

# Remove files not needed in the report.
# Adjust this as needed.
lcov  --remove all.info "/usr/*" "*/test/*" "*/gtest/*" --output report.info

# Generate the HTML webpages from the final lcov trace file.
genhtml -o coverage report.info 
