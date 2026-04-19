cmake -S . -B build -G Ninja
pushd build
ninja && hello.exe
popd
