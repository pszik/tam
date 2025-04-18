#!/usr/bin/env bash

# build
echo "Building..."
(cd ../build && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && cmake --build .) >/dev/null

# run
echo "Running..."
INPUT="38\n18\n"
OUTPUT=$(../build/src/tam gcd.tam <<<"$INPUT")

# check
SUCCESS=$?
if [[ $SUCCESS -ne 0 ]]; then
  echo "execution failed with non-zero return code: $SUCCESS"
  exit 1
fi

if [[ $OUTPUT != "6\n" ]]; then
  echo "incorrect output: \"$OUTPUT\""
  exit 2
fi

exit 0
