# Task 1: Simple graphical app
This application simulates flying ball and randomly generated obstacle, interacting with ball
## Graphial backend
In this app SDL2 used as graphical library, top level api collected from: https://github.com/lisitsynSA/llvm_course/blob/main/SDL
## Build and Run
Build requires:
```
$> sudo apt install libsdl2-dev
$> sudo apt install clang cmake
```
> **Note:** Important! Clang version used in project: 18.1.3

Simple run:
```
$> mkdir build && cd build/
$> cmake ..
$> make
$> ./APP
```
## optimized IR

In folder `LLVM_IR/` you can find `app.ll` - LLVM IR dumped for `app.c` file with `-O3` level of optimizations.
