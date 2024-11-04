set -e

mkdir -p bin
pushd bin
cmake ..
make -j8
popd

bin/plainview
#bin/drmview
