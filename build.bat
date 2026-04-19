cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
pushd build
ninja && hello.exe
popd
