#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

mkdir -p build64 && pushd $_

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..

make VERBOSE=1 -j

make test

popd

mkdir -p java/src/main/resources/natives/osx_64 && cp build64/java/libopenrasp_v8_java.dylib $_

pushd java

mvn test install

popd

rm -rf dist

mkdir dist

tar zcf dist/java_natives_osx.tar.gz -C java/src/main/resources ./natives