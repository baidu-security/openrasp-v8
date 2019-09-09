#!/bin/bash

set -ev
 
ROOT=$(git rev-parse --show-toplevel)

if [ $TRAVIS ]; then
  DIR=$HOME/cache
  mkdir -p $DIR
else
  DIR=/tmp
fi

if [[ "$OSTYPE" == "linux-gnu" ]]; then
  FILENAME="openrasp-v8-prebuilts-linux.tar.gz"
elif [[ "$OSTYPE" == "darwin"* ]]; then
  FILENAME="openrasp-v8-prebuilts-darwin.tar.gz"
elif [[ "$OSTYPE" == "msys" ]]; then
  FILENAME="openrasp-v8-prebuilts-windows.tar.gz"
else
  echo "Unsupported os type"
  exit 1
fi

curl -L -o $DIR/$FILENAME.download -z $DIR/$FILENAME https://packages.baidu.com/app/openrasp/v8/$FILENAME
[[ -f $DIR/$FILENAME.download ]] && mv $DIR/$FILENAME.download $DIR/$FILENAME
tar zxf $DIR/$FILENAME -C $ROOT/prebuilts
