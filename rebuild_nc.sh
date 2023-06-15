#!/bin/sh

#make clean
make || exit 10
make install
cd ./res
tar -cv . | nc -l -p 1111 -q 1
