#!/bin/bash

set -ev

tar zxf vendors/centos6-sysroot.tar.gz -C /tmp/

mkdir -p build64 && pushd $_

(source /tmp/centos6-sysroot/setx64.sh && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..)

make VERBOSE=1 -j

make test

ldd base/tests

popd

mkdir -p build32 && pushd $_

(source /tmp/centos6-sysroot/setx86.sh && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..)

make VERBOSE=1 -j

make test

ldd base/tests

popd

mkdir -p java/src/main/resources/natives/linux_32 && cp build32/java/libopenrasp_v8_java.so $_

mkdir -p java/src/main/resources/natives/linux_64 && cp build64/java/libopenrasp_v8_java.so $_

pushd java

mvn test

popd

rm -rf dist

mkdir dist

tar zcf dist/java_natives_linux.tar.gz -C java/src/main/resources ./natives