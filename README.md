# Performance Challenge #6

See the announcement post [here](https://easyperf.net/blog/2022/05/28/Performance-analysis-and-tuning-contest-6).

# The task

The task is to split a text and count each word's frequency, then print the list sorted by frequency in decreasing order. Ties are printed in alphabetical order.

Example:
```bash
$ echo "apple pear apple art" | ./wordcount.exe
apple   2
art     1
pear    1
```

See additional rules and assumptions in [wordcount.cpp](wordcount.cpp).

# How to set up the environment

Here is the list of tools you *absolutely* have to install to build the benchmark:
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
$ cmake --build . --target validate
```

**Benchmark:**
```bash
$ cmake --build . --target benchmark
```

**Benchmark against baseline:**
```bash
$ python3 check_speedup.py -challenge_path path/to/repo -bench_lib_path ~/workspace/benchmark/benchmark -num_runs 3
```

Keep in mind, the large input file is *very* large. We recommend having at least 8GB RAM.

# Acknowledgments
- Inspired by https://github.com/juditacs/wordcount/
- Idea for the challenge was suggested by Karthik Tadinada.