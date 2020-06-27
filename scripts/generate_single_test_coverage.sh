#!/bin/sh

die() {
    echo "Error: $1" 1>&2
    exit 1
}

[ -n "$1" ] || die "Missing test to execute"

TEST=$1

make -j$(nproc) ${TEST} && \
tests/${TEST} && \
lcov --capture --directory tests/CMakeFiles/${TEST}.dir/ --output-file coverage.info && \
lcov --capture --directory lib/CMakeFiles/woinc.dir/ \
    --directory tests/CMakeFiles/${TEST}.dir/ \
    --output-file coverage.info && \
lcov --remove coverage.info "/usr/include*" --output-file  coverage.info && \
lcov --remove coverage.info "g++*" --output-file  coverage.info && \
genhtml coverage.info --output-directory out || die "Ouch"

