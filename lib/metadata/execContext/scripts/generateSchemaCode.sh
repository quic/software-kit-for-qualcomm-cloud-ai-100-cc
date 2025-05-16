## Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
## SPDX-License-Identifier: BSD-3-Clause-Clear

cd "$(dirname "$0")"
set -ex
cd ../src
deps_path=../deps-generated
rm -rf $deps_path
mkdir -p $deps_path/firmware
mkdir -p $deps_path/flatbuffers
../../tools/flatbuffers/build/flatc --gen-object-api --cpp execContext.fbs --no-warnings
mv execContext_generated.h $deps_path/flatbuffers
../../tools/flatcc/bin/flatcc -a execContext.fbs
../../tools/flatbuffers/build/flatc execContext.fbs --jsonschema --no-warnings
mv execContext.schema.json $deps_path
mv execContext_* $deps_path/firmware
mv flatbuffers_common_* $deps_path/firmware
cd -
python3.8 generateHeader.py generateExecContextHeader
