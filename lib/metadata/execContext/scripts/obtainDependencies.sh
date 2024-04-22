## Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
## SPDX-License-Identifier: BSD-3-Clause-Clear

cd "$(dirname "$0")" #cd to the directory this script is in
cd ..  #put our child shell one level up
# build and install flatbuffers compiler
flatc_compiler_bin=../tools/flatbuffers/build/flatc
if [ ! -f $flatc_compiler_bin ]; then
  cd ../tools/flatbuffers
  cmake -B build -S . -G "Ninja" -DFLATBUFFERS_BUILD_TESTS=Off -DFLATBUFFERS_INSTALL=On -DCMAKE_BUILD_TYPE=Release -DFLATBUFFERS_BUILD_FLATHASH=Off
  cmake --build build --target install
  cd -
fi
# build and install flatcc compiler
flatcc_compiler_bin=../tools/flatcc/bin/flatcc
if [ ! -f $flatcc_compiler_bin ]; then
  cd ../tools/flatcc
  scripts/initbuild.sh make
  scripts/setup.sh -a ../mymonster
  rm -rf ../mymonster
  cd -
fi
