#!/bin/sh

set -e

rm -rf fuzz-data
mkdir -p fuzz-data
make CXX=afl-c++ OUT=./fuzz-data

mkdir -p fuzz-data/seeds
echo '100' > fuzz-data/seeds/number.json
echo '"Hello World"' > fuzz-data/seeds/string.json
echo '[10, 20]' > fuzz-data/seeds/array.json
echo '{foo: "bar"}' > fuzz-data/seeds/object.json
mkdir -p fuzz-data/output

cd fuzz-data

afl-fuzz -i seeds -o output -M "json5-fuzzer-1" -- ./json5-to-json &
for i in $(seq 2 20); do
	afl-fuzz -i seeds -o output -S "json5-fuzzer-$i" -- ./json5-to-json >/dev/null &
done

wait
