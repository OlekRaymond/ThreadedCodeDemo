
To benchmark:
- [X] Labels
- [X] Labels executing inline lambdas (should be little)
    - Massive speedup
- [X] Effect of unreachable
    - Seems negative for some reason(?)
- [ ] Effect of mtune and march
    - codespace: Seems slightly negative 
- [ ] Removing `std::map` in favour of stack
- [ ] Avoiding the syscall of reading the code in.
    - Should be minial as it's cached
- [ ] Moving everything to constexpr
    - benchmarks would have to open a file in setup or later 

---

build with:
```bash
cmake -B folder -S . -DCMAKE_BUILD_TYPE=Release
cmake --build folder --config Release
./folder/benchmark_demo
```
where: 
- `folder` is any folder of your choice, 
    + it will be created for you
    + it can be reused
    + the most common choice is `build`

Note:
- all dependecies will be downloaded into `folder`
- LTO, march and mtune will all be turned on for a release build but no others
- ASAN can be turned on with `-DENABLE_ASAN=ON` in the first command

