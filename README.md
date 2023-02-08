# QAic Compute SDK

The QAic Compute SDK uses the hexagon toolchain and contains the
QAic Compute low level toolchain, runtime, scripts, and a
boilerplate app that are necessary to create executable C/C++
compute applications that will run on the Qualcomm Cloud AI 100.

## Prerequisites

All Dependencies have been tested on an Ubuntu 20.04 system.

### Hexagon Tools
Obtain Hexagon tools from `https://github.com/quic/toolchain_for_hexagon`

Tested version is 15.0.5:
- https://codelinaro.jfrog.io/artifactory/codelinaro-toolchain-for-hexagon/v15.0.5/clang+llvm-15.0.5-cross-hexagon-unknown-linux-musl.tar.xz

### Build dependencies

In order to build, in addition to the base system, you must install:
- ninja-build
- clang
- zlib1g-dev
- dependencies for above
- CMake 3.24 or higher.  Instructions are at https://apt.kitware.com/

## Building and Testing the QAic Compute SDK

From the root of the project run the command:

```
export HEXAGON_TOOLS_DIR=<path>
./scripts/build.sh [--tools-dir <tools-dir>] [--run-ctest] [--install]
```

### build.sh full usage
```
Usage: build.sh [ --debug | --release | --release-assert (default) ]
                [ --tools-dir ]
                [ --run-ctest|--run-tests [--verbose-tests] ]
                [ --install ]

--debug, --release, --release-assert change the build type (release-assert is default)
--tools-dir points to the location of build tools directory (only needed if HEXAGON_TOOLS_DIR is unset)
--run-ctest|run-tests runs ctest after building the project
--verbose-tests Passes --verbose to ctest
--install Installs the build output in the install/<build_type> directory
```

## Using the QAic Compute SDK
### Environment Variables

The installation is fully self contained but requires additional environment
variables to locate the tools.

```bash
export QAIC_COMPUTE_INSTALL_DIR=<path_to_qaic_source>/install/<build_type>
export PATH=${QAIC_COMPUTE_INSTALL_DIR}/exec:$PATH
export HEXAGON_TOOLS_DIR=<path>
```

### CMake QAic Compute Toolchain File

To ease development with CMake, a toolchain file is provided to automatically
set up the appropriate commands for cross compiling a QAIC compute application
from the host.  This assumes an artifact package has been created with build.sh --install.

```
cmake -DCMAKE_TOOLCHAIN_FILE=${QAIC_COMPUTE_INSTALL_DIR}/dev/cmake/qaic.cmake
```

## Examples

The examples directory `<path_to_qaic_source>/examples/compute` contains example CMake
projects and source code that demonstrate how to build a barebones QAIC compute
application.

The example Barebones App only serves as an example for the bare minimum code
needed to compile, link, and generate a library and a binary that uses the library.

### Building Barebones Example Application

Set environment variables
```bash
export QAIC_COMPUTE_INSTALL_DIR=<path_to_qaic_source>/install/<build_type>
export PATH=${QAIC_COMPUTE_INSTALL_DIR}/exec:$PATH
export HEXAGON_TOOLS_DIR=<path>
```

Copy example app to your workspace
```bash
cp -R <path_to_qaic_source>/examples/compute/barebones_app <your workspace>
cd <your workspace>/barebones_app
```

Setup CMake
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=${QAIC_COMPUTE_INSTALL_DIR}/dev/cmake/qaic.cmake ..
```

Build Application
```bash
make
```

At the end of the build step, two binaries should be generated:
```
BarebonesApp.elf
BarebonesApp.qpc
```

### Additional Example Applications

There are additional example applications demonstrating other aspects of operation.  Refer to the
documentation in those examples for details as to how they work and what they are demonstrating.

## License
QAic Compute SDK is licensed under the terms in the [LICENSE](LICENSE) file.