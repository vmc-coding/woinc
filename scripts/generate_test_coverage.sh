#!/bin/sh

die() {
    echo "Error: $1" 1>&2
    exit 1
}

# From http://mywiki.wooledge.org/BashFAQ/004
if (shopt -s nullglob dotglob; f=(*); ((${#f[@]}))); then
    die "The current directory is not empty."
fi

woinc_dir=$(dirname "$0")/..
[ -f "${woinc_dir}"/CMakeLists.txt ] || die "Missing CMakeLists.txt"

if which ninja >/dev/null 2>&1; then
    build_tool=Ninja
    build_cmd=ninja
else
    build_tool='Unix Makefiles'
    build_cmd=make
fi

cmake -G "${build_tool}" -DCMAKE_BUILD_TYPE=dev_release -DWOINC_ENABLE_COVERAGE=ON "${woinc_dir}" && \
${build_cmd} -j$(nproc) check && \
${build_cmd} -j$(nproc) manual-tests && \
lib/tests/manual_posix_socket_tests && \
lcov --capture --directory lib/CMakeFiles/woinc.dir/ \
    --directory lib/tests/CMakeFiles/md5_tests.dir/ \
    --directory lib/tests/CMakeFiles/xml_tests.dir/ \
    --directory lib/tests/CMakeFiles/manual_posix_socket_tests.dir/ \
    --output-file coverage.info && \
lcov --remove coverage.info "/usr/include*" --output-file coverage.info && \
lcov --remove coverage.info "g++*" --output-file coverage.info && \
genhtml coverage.info --output-directory out
