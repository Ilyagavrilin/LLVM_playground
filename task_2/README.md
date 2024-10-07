# PassTraceInstructions
This pass prints execeuted IR instructions and their uses
## Build and Run
now in repo presented c code example, how to run it:
1. Build library with pass:
```
$> sudo apt install llvm
$> clang++ -fPIC -shared -o ./c_test/libPassTraceInstructions.so PassTraceInstructions.cpp \
   `llvm-config --cxxflags --ldflags --system-libs --libs core`
```
2. Apply to code, wich computes factorial:
```
$> clang -O2 -S -fno-slp-vectorize -fno-vectorize -mllvm -unroll-count=2 -emit-llvm ./c_test/fact.c -o ./c_test/fact.ll
$> opt -load-pass-plugin=./c_test/libPassTraceInstructions.so -passes="trace-instruction" -S -o ./c_test/fact_instrumented.ll ./c_test/fact.ll
```
3. Build and run executable
```
$> clang ./c_test/fact_instrumented.ll ./log.c ./c_test/start.c -o ./c_test/fact
$> ./c_test/fact 6
```