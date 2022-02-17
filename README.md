## "Local" compilation (non-web):

Steps are the same on Linux and Windows, but on Windows Visual Studio files will
be generated so might as well just run `cmake ..` and then open the .sln file.

VSCode should be able to do this with the cmake plugin

```bash
mkdir build
cd build
cmake ..
make
```

## Web compilation on Linux:

Only tested on Arch. Run the code through the emscripten compiler by using their
cmake toolchain file by running the `cmake` command below.c

Install emscripten and set up emsdk and the EMSDK environment variable. On my
laptop:
```
echo $EMSDK
/home/david/dev/tools/emsdk
```

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DPLATFORM=Web ..
make
emrun raylib_test.html
```

