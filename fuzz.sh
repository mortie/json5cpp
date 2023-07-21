#!/bin/sh

set -e

rm -rf fuzz-data
mkdir -p fuzz-data
make CXX=afl-c++ DESTDIR=./fuzz-data

mkdir -p fuzz-data/seeds
cp JSONTestSuite/test_parsing/y_* fuzz-data/seeds
mkdir -p fuzz-data/output

cd fuzz-data

afl-fuzz -i seeds -o output -M "json5-fuzzer-1" -- ./json5 &
for i in $(seq 2 20); do
	afl-fuzz -i seeds -o output -S "json5-fuzzer-$i" -- ./json5 >/dev/null &
done

wait
