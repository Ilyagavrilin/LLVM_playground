cmake_minimum_required(VERSION 3.7)
project(ASM_SIM)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_FLAGS "-g -O2")
add_executable(ASM_SIM ../task_1/sim.c app_asm_IRgen_1.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFFI REQUIRED IMPORTED_TARGET libffi)

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    ExecutionEngine
    OrcJIT
    MCJIT
    Interpreter
    MC
    Support
    native  # This enables native target support, if available
)
# target_link_libraries(ASM_SIM ${LLVM_LIBS})
# target_link_libraries(ASM_SIM ${SDL2_LIBRARIES})
target_link_libraries(ASM_SIM ${LLVM_LIBS} ${SDL2_LIBRARIES} PkgConfig::LIBFFI)