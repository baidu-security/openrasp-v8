#!/bin/bash

set -ex

cat console.js checkpoint.js context.js flex.js rasp.js > builtins

xxd -i builtins > builtins.h

rm builtins