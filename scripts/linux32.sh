#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

rm -rf build32

mkdir -p build32 && pushd $_

(if [[ -d /tmp/centos6-sysroot ]]; then source /tmp/centos6-sysroot/setx86.sh; fi; cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=all ..)

make VERBOSE=1 -j

./base/tests -s

ldd base/tests

popd

mkdir -p java/src/main/resources/natives/linux_32 && cp build32/java/libopenrasp_v8_java.so $_

# pushd java

# mvn test