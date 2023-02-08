#!/bin/bash

set -euo pipefail # be more strict about non-zero exits and unset var substitutions

usage() {
  cat << EOM
Usage: build.sh [ --debug | --release | --release-assert (default) ]
                [ --tools-dir ]
                [ --run-ctest|--run-tests [--verbose-tests] ]
                [ --install ]

--debug, --release, --release-assert change the build type (release-assert is default)
--tools-dir points to the location of build tools directory
--run-ctest|run-tests runs ctest after building the project
--verbose-tests Passes --verbose to ctest
--install Installs the build output in the install directory

EOM
}

scriptDir="$(readlink -f "$(dirname "$0")")"
srcDir="$(readlink -f $scriptDir/..)"
toolsDir=${AIC_TOOLS_DIR:-$srcDir/..}

DRY_RUN=0
run_echo() {
  if [ $DRY_RUN -eq 1 ]; then
    echo "$@"
  else
    "$@"
  fi
}

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
BUILD_TYPE="Release"
ENABLE_ASSERTIONS="ON"
RUN_CTEST="OFF"
CTEST_ARGS=("--output-on-failure")
DO_INSTALL="OFF"

# Parse options.
while [ $# -gt 0 ]; do
    if [ z"$1" == "z--debug" ]; then
        BUILD_TYPE="Debug"
        ENABLE_ASSERTIONS="ON"
        shift
    elif [ z"$1" == "z--release" ]; then
        BUILD_TYPE="Release"
        ENABLE_ASSERTIONS="OFF"
        shift
    elif [ z"$1" == "z--release-assert" ]; then
        BUILD_TYPE="Release"
        ENABLE_ASSERTIONS="ON"
        shift
    elif [ z"$1" == "z--tools-dir" ]; then
        toolsDir=$2
        shift 2
    elif [ z"$1" == "z--run-ctest" ]; then
        RUN_CTEST="ON"
        shift
    elif [ z"$1" == "z--run-tests" ]; then
        RUN_CTEST="ON"
        shift
    elif [ z"$1" == "z--verbose-tests" ]; then
        CTEST_ARGS+=("--verbose")
        shift
    elif [ z"$1" == "z--install" ]; then
        DO_INSTALL="ON"
        shift
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

# Set the hexagon tools to be used
export HEXAGON_TOOLS_DIR=${HEXAGON_TOOLS_DIR:-$toolsDir}
if [ ! -d ${HEXAGON_TOOLS_DIR} ]; then
    echo -n "Error: could not find hexagon tools directory: "
    echo ${HEXAGON_TOOLS_DIR}
    exit 1
fi
if [ ! -f ${HEXAGON_TOOLS_DIR}/bin/clang++ ]; then
    echo -n "Error: could not find bin/clang++ within hexagon tools directory: "
    echo ${HEXAGON_TOOLS_DIR}
    exit 1
fi

BUILDDIR_SUFFIX="-${BUILD_TYPE,,}"
if [ z${BUILD_TYPE} == "zRelease" ] && [ z${ENABLE_ASSERTIONS} == "zON" ]; then
    BUILDDIR_SUFFIX="${BUILDDIR_SUFFIX}-assert"
fi


BUILD_DIR="${srcDir}/build/qaic-compute${BUILDDIR_SUFFIX}"
INSTALL_DIR="${srcDir}/install/qaic-compute${BUILDDIR_SUFFIX}"

LLVM_INSTALL_SUFFIX=${BUILDDIR_SUFFIX}
# We don't have an LLVM debug build, so use release-assert instead
if [ $LLVM_INSTALL_SUFFIX == "-debug" ]; then
    LLVM_INSTALL_SUFFIX="-release-assert"
fi

## Ensure local dependencies are available in this environment
search_for_exes cmake ninja clang clang++

if [ ${BUILD_TYPE} == "Release" ]; then
    AC_CFLAGS="-O2"
    AC_CXXFLAGS="-O2 -std=c++11"
elif [ ${BUILD_TYPE} == "Debug" ]; then
    AC_CFLAGS="-O0 -g"
    AC_CXXFLAGS="-O0 -g -std=c++11"
else
    echo "Error: Unknown build type."
    exit 1
fi

if [ ${ENABLE_ASSERTIONS} == "OFF" ]; then
    AC_CFLAGS="${AC_CFLAGS} -DNDEBUG"
    AC_CXXFLAGS="${AC_CXXFLAGS} -DNDEBUG"
fi

## Ensure zlib are available
if [ ! -f /usr/include/zlib.h ]; then
    echo "Unable to find zlib. Exiting."
    exit 1
fi

run_echo mkdir -p ${BUILD_DIR}/tmp
run_echo pushd ${BUILD_DIR}

CMAKE_ARGS=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=clang++")
CMAKE_ARGS+=("-DCMAKE_C_COMPILER=clang")
CMAKE_ARGS+=("-DCMAKE_INSTALL_MESSAGE=LAZY")
CMAKE_ARGS+=("-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}")
CMAKE_ARGS+=("-DHEXAGON_TOOLS_DIR=${HEXAGON_TOOLS_DIR}")
CMAKE_ARGS+=("-DQAIC_ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS}")
CMAKE_ARGS+=("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")

# Disable 3rd party tests
CMAKE_ARGS+=("-DFLATBUFFERS_BUILD_TESTS=OFF")

run_echo cmake -G Ninja ${CMAKE_ARGS[*]} ${SRC_DIR}
run_echo ninja all

# Add Paths to qaic tools for toolchain unit tests to find
PATH=${BUILD_DIR}/tools/qaic-cc:${PATH}
PATH=${BUILD_DIR}/tools/qaic-ar:${PATH}
PATH=${BUILD_DIR}/tools/qaic-objcopy:${PATH}
export PATH

if [ ${RUN_CTEST} == "ON" ]; then
    run_echo ctest ${CTEST_ARGS[*]}
fi

if [ ${DO_INSTALL} == "ON" ]; then
    run_echo ninja install
fi
