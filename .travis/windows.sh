#!/bin/bash

set -ev

choco install -y jdk8 -params "both=true" --version 8.0.211

choco install -y maven --version 3.6.1

mkdir -p build64 && pushd $_

cmake -DCMAKE_VERBOSE_MAKEFILE=ON -Ax64 -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..

cmake --build . --config RelWithDebInfo

cmake --build . --config RelWithDebInfo --target RUN_TESTS

popd

mkdir -p build32 && pushd $_

cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DBUILD_TESTING=ON -DENABLE_LANGUAGES=java ..

cmake --build . --config RelWithDebInfo

cmake --build . --config RelWithDebInfo --target RUN_TESTS

popd

mkdir -p java/src/main/resources/natives/windows_32 && cp build32/java/RelWithDebInfo/openrasp_v8_java.dll $_

mkdir -p java/src/main/resources/natives/windows_64 && cp build64/java/RelWithDebInfo/openrasp_v8_java.dll $_

pushd java

JAVA_HOME="/c/Program Files (x86)/Java/jdk1.8.0_211" mvn test install

JAVA_HOME="/c/Program Files/Java/jdk1.8.0_211" mvn test install

popd

rm -rf dist

mkdir dist

tar zcf dist/java_natives_windows.tar.gz -C java/src/main/resources ./natives
