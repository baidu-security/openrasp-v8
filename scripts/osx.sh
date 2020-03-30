#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

rm -rf build64

mkdir -p build64 && pushd $_

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=all ..

make VERBOSE=1 -j

./base/tests -s

popd

mkdir -p java/src/main/resources/natives/osx_64 && cp build64/java/libopenrasp_v8_java.dylib $_

pushd java

mvn test
