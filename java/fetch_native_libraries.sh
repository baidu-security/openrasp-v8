#!/bin/bash

set -ev
ROOT=$(git rev-parse --show-toplevel)
TAG=$(git describe --tags --abbrev=0)
if [ $TRAVIS ]; then
  DIR=$HOME/cache
  mkdir -p $DIR
else
  DIR=/tmp
fi
echo $ROOT $TAG $DIR
curl -L -o $DIR/java_natives_linux.tar.gz.download -z $DIR/java_natives_linux.tar.gz https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_linux.tar.gz
mv $DIR/java_natives_linux.tar.gz.download $DIR/java_natives_linux.tar.gz || true
tar zxf $DIR/java_natives_linux.tar.gz -C $ROOT/java/src/main/resources
curl -L -o $DIR/java_natives_osx.tar.gz.download -z $DIR/java_natives_osx.tar.gz https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_osx.tar.gz
mv $DIR/java_natives_osx.tar.gz.download $DIR/java_natives_osx.tar.gz || true
tar zxf $DIR/java_natives_osx.tar.gz -C $ROOT/java/src/main/resources
curl -L -o $DIR/java_natives_windows.tar.gz.download -z $DIR/java_natives_windows.tar.gz https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_windows.tar.gz
mv $DIR/java_natives_windows.tar.gz.download $DIR/java_natives_windows.tar.gz || true
tar zxf $DIR/java_natives_windows.tar.gz -C $ROOT/java/src/main/resources
