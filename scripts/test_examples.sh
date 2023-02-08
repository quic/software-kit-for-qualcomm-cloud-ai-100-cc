#!/bin/bash

# Assumes the hexagon tool directory for HEXAGON_TOOLS_DIR will be provided
# as the first argument to the script if the environment variable is not set.
# Also assumes qaic-runner is in PATH

set -euo pipefail # be more strict about non-zero exits and unset var substitutions

scriptDir="$(readlink -f "$(dirname "$0")")"
srcDir="$(readlink -f $scriptDir/..)"

QAIC_COMPUTE_INSTALL_DIR=${QAIC_COMPUTE_INSTALL_DIR:-$srcDir/install/qaic-compute-release-assert}
PATH=${QAIC_COMPUTE_INSTALL_DIR}/exec:$PATH
export HEXAGON_TOOLS_DIR=${HEXAGON_TOOLS_DIR:-$1}

function pushd {
    command pushd "$@" > /dev/null
}

function popd {
    command popd "$@" > /dev/null
}

function buildTest() {
    testName=$1
    echo
    echo "Attempting to build $testName"
    rm -rf ./$testName
    cp -r $QAIC_COMPUTE_INSTALL_DIR/examples/compute/$testName ./$testName
    pushd ./$testName
    mkdir build
    pushd build
    cmake -DCMAKE_TOOLCHAIN_FILE=${QAIC_COMPUTE_INSTALL_DIR}/dev/cmake/qaic.cmake ..
    set +e
    make && ls | grep -q '.qpc'
    rc=$?
    set -e

    if [ $rc -ne 0 ]; then
        echo
        echo "[FAILED] to build $testName QPC"
        exit 1
    fi
    popd
    popd
    echo
    echo "Build completed"
}

function getInputs() {
    case $1 in
        "simple_io")
            echo "-i ../buffer1.txt -i ../buffer2.txt"
            ;;
        "multicast_example")
            echo "-i ../input1.txt -i ../input2.txt"
            ;;
        *)
            echo "$1 isn't a runnable test"
            exit 1
            ;;
    esac
}

checkOutputs() {
    case $1 in
        "simple_io")
            ;&
        "multicast_example")
            diff ../expected_output.txt output/*.bin
            return $?
            ;;
        *)
            echo "$1 isn't a runnable test"
            exit 1
            ;;
    esac
}

runTest() {
    testName=$1
    echo
    echo "Attempting to run $testName"
    pushd $testName/build
    ln -s *.qpc programqpc.bin
    set +e
    qaic-runner -t . $(getInputs $testName) --write-output-dir output --write-output-num-samples 1 --write-output-start-iter 0 --num-iter 1
    if [ $? -ne 0 ]; then
        echo
        echo "[FAILED] to run $testName"
        exit 1
    fi
    checkOutputs $testName
    if [ $? -ne 0 ]; then
        echo
        echo "[FAILED] output check for $testName"
        exit 1
    fi
    set -e
    popd
    echo
    echo "Test completed"
}

function cleanTest() {
    testName=$1
    rm -rf ./$testName
}

# Check the Cmake version
set +e
cmakeVer=$(cmake --version)
cmakeRC=$?
set -e
if [ $cmakeRC != 0 ]; then
    echo
    echo "[ERROR] Please set CMAKE to a valid executable"
    exit 1
elif [ $(echo $cmakeVer | sed -n -e "s/.*version 3.\([0-9]\+\).*/\1/p") -lt 12 ]; then
    echo
    echo "[ERROR] Please set CMAKE to a version greater than or equal to 3.12"
    exit 1
fi

# Copy and Build tests
buildTest "barebones_app"
buildTest "simple_io"
buildTest "multicast_example"

echo
echo "[PASSED] all builds"

# Run tests if possible
if `which qaic-runner > /dev/null`; then
    runTest "simple_io"
    runTest "multicast_example"

    echo
    echo "[PASSED] all tests"
else
    echo
    echo "[SKIPPED] running of tests due to not finding qaic-runner"
fi

# Clean the tree so we don't accidentally commit the test artifacts
cleanTest "barebones_app"
cleanTest "simple_io"
cleanTest "multicast_example"

exit 0
