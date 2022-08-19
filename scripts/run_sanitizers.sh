#!/usr/bin/bash

cd "${ZIP_NAME}"
cat "${INPUT_FILENAME}" | tr -d '\r' | ./"${EXECUTABLE_NAME}" &
bash ../scripts/run_test.sh 3 1 2
