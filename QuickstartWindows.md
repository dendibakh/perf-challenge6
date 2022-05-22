## Set up environment on Windows

1. Run powershell.

2. Install 7zip from [here](https://www.7-zip.org/download.html). 

    Add it to PATH. For example:

    ```
    $ENV:PATH="$ENV:PATH;C:\Program Files\7-Zip"
    ```

3. Run `./create_input.ps1`. It may take a couple of minutes.

4. Install [ninja](https://github.com/ninja-build/ninja/releases). 
    
    Add it to the PATH. For example:
    ```
    $ENV:PATH="$ENV:PATH;C:\Program Files\ninja"
    ```
5. Download clang-14 compiler from [here](https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.0) (LLVM-14.0.0-win64.exe) and install it. Select "add LLVM to the PATH" while installing.

6. Build release version of google [benchmark library](https://github.com/google/benchmark#installation). It doesn't matter which compiler you use to build it. Install google benchmark library with:
    ```
    cmake --build "build" --config Release --target install
    ```
    Add google benchmark library to PATH
    ```
    $ENV:PATH="$ENV:PATH;C:\Program Files (x86)\benchmark\lib"
    ```

7. If everything works as expected, you can set environment variables permanently (run as Administrator):
    ```
    # be carefull, back up your PATH
    setx /M PATH "$($env:path);C:\Program Files\7-Zip"
    setx /M PATH "$($env:path);C:\Program Files\ninja"
    setx /M PATH "$($env:path);C:\Program Files (x86)\benchmark\lib"
    ```

