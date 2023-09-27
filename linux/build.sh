#!/bin/bash

set -e

apt update
apt install -yq build-essential cmake
cd /PinkRabbitMQ/linux
mkdir build || true
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . && \
cp ../test.conf.example ./test.conf
./unittest