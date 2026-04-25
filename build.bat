cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
pushd build
cp compile_commands.json ../compile_commands.json
ninja && hello.exe
popd
