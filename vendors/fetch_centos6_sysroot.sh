#!/bin/bash

set -ev

if [ $TRAVIS ]; then
  DIR=$HOME/cache
  mkdir -p $DIR
else
  DIR=/tmp
fi

curl -# -k -L -o $DIR/centos6-sysroot.tar.gz.download -z $DIR/centos6-sysroot.tar.gz https://packages.baidu.com/app/openrasp/v8/centos6-sysroot.tar.gz
[[ -f $DIR/centos6-sysroot.tar.gz.download ]] && mv $DIR/centos6-sysroot.tar.gz.download $DIR/centos6-sysroot.tar.gz
tar zxf $DIR/centos6-sysroot.tar.gz -C /tmp/