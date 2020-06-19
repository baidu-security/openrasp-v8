#!/bin/bash

set -e
 
ROOT=$(git rev-parse --show-toplevel || pwd)

if [ $TRAVIS ]; then
  DIR=$HOME/cache
  mkdir -p $DIR
else
  DIR=/tmp
fi
FILENAME="static-lib.tar.gz"
curl -# -k -L -o $DIR/$FILENAME.download https://packages.baidu.com/app/openrasp/$FILENAME
[[ -f $DIR/$FILENAME.download ]] && mv $DIR/$FILENAME.download $DIR/$FILENAME
tar zxf $DIR/$FILENAME -C $ROOT/prebuilts/linux
