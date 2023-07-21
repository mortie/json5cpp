#!/bin/sh

set -e

rm -rf fuzz-data
mkdir -p fuzz-data
make CXX=afl-c++ DESTDIR=./fuzz-data

mkdir -p fuzz-data/seeds
mkdir -p fuzz-data/output

cd fuzz-data

afl-fuzz -o output -M "json5-fuzzer-1" -- ./json5 &
for i in $(seq 2 20); do
	afl-fuzz -o output -S "json5-fuzzer-$i" -- ./json5 >/dev/null &
done

wait
