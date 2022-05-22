## Set up environment on Linux

1. Run terminal.

2. Run `./create_input.sh`. It may take a couple of minutes.

3. Install [ninja](https://github.com/ninja-build).
    
    ```
    $ sudo apt install ninja-build
    ```

4. Install clang-14 compiler using instructions from [here](https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.0):

    ```
    wget https://apt.llvm.org/llvm.sh
    chmod +x llvm.sh
    sudo ./llvm.sh 14
    ```

5. Build release version of google [benchmark library](https://github.com/google/benchmark#installation). It doesn't matter which compiler you use to build it. Install google benchmark library with:
    ```
    cmake --build "build" --config Release --target install
    ```

6. Enable clang-14 compiler for building labs. If you want to make clang-14 to be the default on a system do the following:
    ```
    sudo update-alternatives --install /usr/bin/cc cc /usr/bin/clang-14 30
    sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-14 30
    ```

    If you don't want to make it a default, you can pass `-DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14` to the CMake.
