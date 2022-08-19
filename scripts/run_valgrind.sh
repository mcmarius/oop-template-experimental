#!/usr/bin/bash

# remove --show-leak-kinds=all (and --track-origins=yes) if there are many leaks in external libs
cd "${ZIP_NAME}"
cat "${INPUT_FILENAME}" | tr -d '\r' | valgrind --leak-check=full --track-origins=yes --error-exitcode=0 ./"${EXECUTABLE_NAME}" &
bash ../scripts/run_test.sh 10 1 2
