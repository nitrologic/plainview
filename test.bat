pushd bin
cmake ..
ninja -j8
popd

bin\plainview.exe
