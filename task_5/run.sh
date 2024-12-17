#!/bin/bash

# Move to the directory containing this script
cd $(dirname "$0")
set -x  # Enable command tracing

# Step 1: Generate the parser and scanner
bison -d parser.y || exit 1
lex lexer.l || exit 1

# Step 2: Compile the generated files with LLVM and Clang
clang++ lex.yy.c parser.tab.c $(llvm-config --cppflags --ldflags --libs) -o mglc || exit 1

# Step 3: Clean up temporary files
rm parser.tab.c parser.tab.h lex.yy.c

# Step 4: Provide input code and run the parser
echo "===    MiniGraphLang programm    ==="
cat examples/program.mgl
echo "=== Running MiniGraphLang Parser ==="
cat examples/program.mgl | ./mglc