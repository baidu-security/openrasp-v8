#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

rm -rf build64

mkdir -p build64 && pushd $_

(if [[ -d /tmp/centos6-sysroot ]]; then source /tmp/centos6-sysroot/setx64.sh; fi; cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=all ..)

make VERBOSE=1 -j

./base/tests -s

ldd base/tests

popd

mkdir -p java/src/main/resources/natives/linux_64 && cp build64/java/libopenrasp_v8_java.so $_

pushd java

mvn test