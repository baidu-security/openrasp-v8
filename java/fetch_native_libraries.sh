#!/bin/bash

set -ev
ROOT=$(git rev-parse --show-toplevel)
TAG=$(git describe --tags --abbrev=0)
if [ $TRAVIS ]; then
  DIR=$HOME/cache
else
  DIR=/tmp
fi
wget -N -P $DIR https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_linux.tar.gz
wget -N -P $DIR https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_osx.tar.gz
wget -N -P $DIR https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_windows.tar.gz
tar zxf $DIR/java_natives_linux.tar.gz -C $ROOT/java/src/main/resources
tar zxf $DIR/java_natives_osx.tar.gz -C $ROOT/java/src/main/resources
tar zxf $DIR/java_natives_windows.tar.gz -C $ROOT/java/src/main/resources
