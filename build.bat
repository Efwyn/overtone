cmake -S . -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
pushd build_debug
move compile_commands.json ../compile_commands.json
ninja && hello.exe
popd
