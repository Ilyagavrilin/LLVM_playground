cmake_minimum_required(VERSION 3.13)
project(MiniGraphLang LANGUAGES CXX)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Bison
set(BISON_OUTPUT_C ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c)
set(BISON_OUTPUT_H ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.h)

add_custom_command(
    OUTPUT ${BISON_OUTPUT_C} ${BISON_OUTPUT_H}
    COMMAND bison -d ${CMAKE_SOURCE_DIR}/parser.y -o ${BISON_OUTPUT_C}
    DEPENDS ${CMAKE_SOURCE_DIR}/parser.y
    COMMENT "Running bison on parser.y"
)

add_custom_target(bison_target
    DEPENDS ${BISON_OUTPUT_C} ${BISON_OUTPUT_H}
    COMMENT "Bison target"
)

# Flex
set(FLEX_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
add_custom_command(
    OUTPUT ${FLEX_OUTPUT}
    COMMAND flex -o${FLEX_OUTPUT} ${CMAKE_SOURCE_DIR}/lexer.l
    DEPENDS ${CMAKE_SOURCE_DIR}/lexer.l
    COMMENT "Running flex on lexer.l"
)

add_custom_target(flex_target
    DEPENDS ${FLEX_OUTPUT}
    COMMENT "Flex target"
)

# Main executable
add_executable(mglc ${BISON_OUTPUT_C} ${BISON_OUTPUT_H} ${FLEX_OUTPUT})

# Ensure generated sources are treated as C++
set_source_files_properties(${BISON_OUTPUT_C} ${FLEX_OUTPUT} PROPERTIES LANGUAGE CXX)

llvm_map_components_to_libnames(REQ_LLVM_LIBS core orcjit native)
target_link_libraries(mglc ${REQ_LLVM_LIBS})

# Make mglc depend on the generation of parser and lexer
add_dependencies(mglc bison_target flex_target)

# Add a custom target to run the example after build
add_custom_target(run_tests
  COMMAND ${CMAKE_COMMAND} -E echo "===    MiniGraphLang programm    ==="
  COMMAND cat ${CMAKE_SOURCE_DIR}/examples/program.mgl
  COMMAND ${CMAKE_COMMAND} -E echo "=== Running MiniGraphLang Parser ==="
  COMMAND mglc < ${CMAKE_SOURCE_DIR}/examples/program.mgl
  COMMAND ${CMAKE_COMMAND} -E echo "===    MiniGraphLang programm    ==="
  COMMAND cat ${CMAKE_SOURCE_DIR}/examples/program_2.mgl
  COMMAND ${CMAKE_COMMAND} -E echo "=== Running MiniGraphLang Parser ==="
  COMMAND mglc < ${CMAKE_SOURCE_DIR}/examples/program_2.mgl
  COMMAND ${CMAKE_COMMAND} -E echo "===    MiniGraphLang programm    ==="
  COMMAND cat ${CMAKE_SOURCE_DIR}/examples/program_3.mgl
  COMMAND ${CMAKE_COMMAND} -E echo "=== Running MiniGraphLang Parser ==="
  COMMAND mglc < ${CMAKE_SOURCE_DIR}/examples/program_3.mgl
  DEPENDS mglc
  COMMENT "Running examples with mglc"
)