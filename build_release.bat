cmake -S . -B build_release -G Ninja -DCMAKE_BUILD_TYPE=Release
pushd build_release
ninja && hello.exe
popd
