cmake_minimum_required(VERSION 3.7)
project(PassTraceInstructions)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")



separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})


set(PASS_SOURCE ${PassTraceInstructions_SOURCE_DIR}/PassTraceInstructions.cpp)
set(PASS_LIB ${CMAKE_CURRENT_BINARY_DIR}/PassTraceInstructions.so)
set(BITCODE_NAME app)
set(DEFAULT_BITCODE ${CMAKE_CURRENT_BINARY_DIR}/${BITCODE_NAME}.ll)
set(INSTRUMENTED_BITCODE ${CMAKE_CURRENT_BINARY_DIR}/${BITCODE_NAME}.bc)
set(OUTPUT_EXECUTABLE instrumented_app)
set(APPLICATION_DIR ${PassTraceInstructions_SOURCE_DIR}/../task_1)
set(SOURCE_PROGRAM ${APPLICATION_DIR}/app.c)
set(SOURCES ${APPLICATION_DIR}/start.c ${APPLICATION_DIR}/sim.c)
set(HELPERS ${PassTraceInstructions_SOURCE_DIR}/log.c)

# Pass building
add_custom_command(
    OUTPUT ${PASS_LIB}
    COMMAND ${CMAKE_CXX_COMPILER} -fPIC -shared -o ${PASS_LIB} ${PASS_SOURCE} `llvm-config --cxxflags --ldflags --system-libs --libs core`
    DEPENDS ${PASS_SOURCE}
    COMMENT "Сборка библиотеки PassTraceInstructions"
)

add_custom_target(PassTraceInstructions ALL
    DEPENDS ${PASS_LIB}
)

# Bitcode generation
add_custom_command(
    OUTPUT  ${DEFAULT_BITCODE}
    COMMAND ${CMAKE_C_COMPILER} -O3 -emit-llvm -c ${SOURCE_PROGRAM} -o ${DEFAULT_BITCODE}
    DEPENDS ${SOURCE_PROGRAM}
    COMMENT "Compiling ${SOURCE_PROGRAM} to ${DEFAULT_BITCODE}"
)

add_custom_target(CompileProgramBitcode ALL
    DEPENDS ${DEFAULT_BITCODE}
)

#Pass applying
add_custom_command(
    OUTPUT ${INSTRUMENTED_BITCODE}
    COMMAND ${LLVM_TOOLS_BINARY_DIR}/opt -load-pass-plugin=${PASS_LIB} -passes="trace-instruction" -S ${DEFAULT_BITCODE} -o ${INSTRUMENTED_BITCODE}
    DEPENDS PassTraceInstructions CompileProgramBitcode
    COMMENT "Applying PassTraceInstructions to ${DEFAULT_BITCODE}"
)

add_custom_target(ApplyPass ALL
    DEPENDS ${INSTRUMENTED_BITCODE}
)

# Executable building
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_EXECUTABLE}
    COMMAND ${CMAKE_C_COMPILER} ${INSTRUMENTED_BITCODE} ${SOURCES} ${HELPERS} -lSDL2 -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_EXECUTABLE}
    DEPENDS ApplyPass
    COMMENT "Generating ${OUTPUT_EXECUTABLE}"
)

add_custom_target(GenerateExecutable ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_EXECUTABLE}
)

