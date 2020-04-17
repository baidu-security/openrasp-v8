#!/bin/bash

set -ev

pushd `git rev-parse --show-toplevel`

if [[ ! -f "/etc/alpine-release" ]]; then
  pushd vendors/alpine-env
  curl -# -k -L -z apache-maven-3.6.3-bin.tar.gz -O https://downloads.apache.org/maven/maven-3/3.6.3/binaries/apache-maven-3.6.3-bin.tar.gz
  curl -# -k -L -z cmake-3.17.1-Linux-musl.tar.gz -O https://packages.baidu.com/app/cmake-3.17.1-Linux-musl.tar.gz
  docker build -t openrasp-v8-alpine-env .
  popd
  docker run --rm -v `pwd`:/openrasp-v8 -w /openrasp-v8 openrasp-v8-alpine-env "/openrasp-v8/scripts/linuxmusl64.sh"
  exit 0
fi

rm -rf build64

mkdir -p build64 && pushd $_

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DENABLE_LANGUAGES=all ..

make VERBOSE=1 -j

./base/tests -s

ldd base/tests

popd

mkdir -p java/src/main/resources/natives/linux_musl64 && cp build64/java/libopenrasp_v8_java.so $_

pushd java

mvn test