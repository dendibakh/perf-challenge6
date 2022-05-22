# Performance Challenge #6
TODO: describe the challenge.

# How to set up the environment

Here is the list of tools you *absolutely* have to install to build labs in this video course:
* CMake 3.13 or higher
* [Google benchmark](https://github.com/google/benchmark).

Next steps depend on your platform of choice. So far we support native builds on Windows and Linux. Check out the instructions specific to each platform ([Windows](QuickstartWindows.md)) ([Linux](QuickstartLinux.md)).

# How to build & run

**Build:**
```bash
$ cmake -E make_directory build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_C_COMPILER="clang" -G Ninja ..
$ cmake --build . --config Release --parallel 8
```

**Validate:**
```bash
$ cmake --build . --target validateLab
```

**Benchmark:**
```bash
$ cmake --build . --target benchmarkLab
```

**Benchmark against baseline:**
```bash
$ python3 check_speedup.py -challenge_path path/to/repo -bench_lib_path ~/workspace/benchmark/benchmark -num_runs 3
```

# Acknowledgments
- Inspired by https://github.com/juditacs/wordcount/
- Idea for the challenge was suggested by Karthik Tadinada.