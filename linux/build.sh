#!/bin/bash

set -e

cd /PRMQ/linux
rm -r ./tmp | true
mkdir tmp
cd tmp
echo build $1 $2 $3
if [ "$1" == "Debug" ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    cmake -DCMAKE_BUILD_TYPE=Release -DVERSION=$2 -DNAME_POSTFIX=$3 ..
fi
cmake --build .

if [ "$1" == "Debug" ]; then
    RMQ_HOST=rabbitmq ctest -V
fi
