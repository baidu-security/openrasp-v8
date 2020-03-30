#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

rm -rf build32

mkdir -p build32 && pushd $_

cmake -DCMAKE_VERBOSE_MAKEFILE=ON -A Win32 -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..

cmake --build . --config RelWithDebInfo

cmake --build . --config RelWithDebInfo --target RUN_TESTS

file java/RelWithDebInfo/openrasp_v8_java.dll

popd

mkdir -p java/src/main/resources/natives/windows_32 && cp build32/java/RelWithDebInfo/openrasp_v8_java.dll $_

# pushd java

# mvn test
