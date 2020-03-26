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
for OS in linux osx windows
do
  curl -# -k -L -o $DIR/java_natives_$OS.tar.gz.download -z $DIR/java_natives_$OS.tar.gz https://github.com/baidu-security/openrasp-v8/releases/download/$TAG/java_natives_$OS.tar.gz
  [[ -f $DIR/java_natives_$OS.tar.gz.download ]] && mv $DIR/java_natives_$OS.tar.gz.download $DIR/java_natives_$OS.tar.gz
  tar zxf $DIR/java_natives_$OS.tar.gz -C $ROOT/java/src/main/resources
done
