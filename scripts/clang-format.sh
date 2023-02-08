#!/bin/bash

set -euo pipefail # be more strict about non-zero exits and unset var substitutions

usage() {
  cat << EOM
Usage: clang-format.sh [ --tools-dir ]

--tools-dir points to the location of build tools directory

EOM
}

scriptDir="$(readlink -f "$(dirname "$0")")"
srcDir="$(readlink -f $scriptDir/..)"
toolsDir=${AIC_TOOLS_DIR:-$srcDir/..}

search_for_exes() {
  if [ $# -gt 0 ]; then
    problem=0
    for exe in "$@"; do
      if ! which $exe > /dev/null 2>&1; then
        problem=1
        echo "$exe not found on path. Ensure this host has it accessible."
      fi
    done
    if [ $problem -eq 1 ]; then
      echo "Used PATH was PATH=$PATH"
      return 1
    fi
  fi
}

function pushd {
    command pushd "$@" > /dev/null
}

function popd {
    command popd "$@" > /dev/null
}

# Default config parameters


# Parse options.
while [ $# -gt 0 ]; do
    if [ z"$1" == "z--tools-dir" ]; then
        toolsDir=$2
        shift 2
    elif [ "z$1" == "z-h" ] || [ "z$1" == "z--help" ]; then
        usage
        exit 1
    else
        break
    fi
done

if [ $# -eq 0 ]; then
    SRC_DIR=$(pwd)
else
    SRC_DIR=$1
    shift
fi

# Canonicalize directories so cmake arg check won't get false positives.
toolsDir=$(readlink -f $toolsDir)
SRC_DIR=$(readlink -f $SRC_DIR)

PATH=${toolsDir}/build_tools/clang8-centos7.6-rtti-release-gcc74lib/bin
PATH=$PATH:/bin
PATH=$PATH:/usr/bin
export PATH

directories=()
directories+=("lib/driver")
directories+=("lib/program")

pushd ${SRC_DIR}

search_for_exes clang-format
find . -type f \( -name \*.h -o -name \*.cpp \) \
      ! -path "./.git/*" \
      ! -path "./build/*" \
      ! -path "./install/*" \
      ! -path "./external/*" \
      ! -path "./legacy/*" \
      ! -path "./lib/networkdesc/*" \
      ! -path "./lib/metadata/*" \
      -exec clang-format -i {} \;
