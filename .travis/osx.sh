#!/bin/bash

set -ev

mkdir -p build64 && pushd $_

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..

make VERBOSE=1 -j

make test

popd

mkdir -p java/src/main/resources/natives/osx_64 && cp build64/java/libopenrasp_v8_java.dylib $_

pushd java

mvn test

popd

pushd java/src/main/resources

tar zcf java_natives_osx.tar.gz natives

popd