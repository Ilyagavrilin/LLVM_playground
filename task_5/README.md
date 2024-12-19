# MiniGraphLang

MiniGraphLang is a simple graphics-oriented language designed for introductory compiler and LLVM IR generation exercises. It allows you to define functions, draw pixels, circles, and rectangles on a screen, and manipulate variables and control flow using `if`, `else`, and `goto`. The language is compiled down to LLVM IR, providing a learning experience in compiler construction and code generation.

> [!IMPORTANT]
> To succesfully build the task it is better to use `nix-shell`.  
> Take a look on main README.md



## Build and Run
```
$> cmake -B build/
$> cd build/
$> make # to compile mglc
$> make run_tests # to run examples IR generation
```

> [!IMPORTANT]
> Task draft, some parts work not dully correct
> ExecutionEngine has been deleted due to some errors
> Have some issues with branches (will be fixed soon)

## Features

- **Functions**: Define functions including a main entry point.
- **Variables**: Declare and assign integer variables.
- **Screen Operations**: 
  - Draw individual pixels with `screen << pixel(...)`.
  - Draw circles with `screen << circle(...)`.
  - Draw rectangles with `screen << rectangle(...)`.
  - Flush the screen with `flush`.
- **Control Flow**: 
  - Conditional branching with `if (...) { ... } else { ... }`.
  - Labels and `goto` for unconditional jumps.
- **Integer Expressions**: Perform arithmetic (`+`, `*`) and comparisons (`<`, `>`) on integers.

## Grammar Overview

Below is a simplified overview of the grammar. The actual grammar may be more detailed and defined using Bison specifications.

**Terminals:**
- `func`, `main`, `var`, `screen`, `pixel`, `circle`, `rectangle`, `flush`, `rand`, `if`, `else`, `goto`
- `<<`, `(`, `)`, `{`, `}`, `,`, `;`, `=`, `<`, `>`, `:`
- Integer literals and identifiers

**Non-Terminals:**
- `Program`: The top-level entry point of the compilation unit.
- `MainFunction`: Defines the `func main { ... }` block.
- `FunctionList`: Zero or more additional function definitions.
- `Function`: A user-defined function `func identifier { ... }`.
- `Statements`: Zero or more statements.
- `Statement`: One of the supported statements (variable assignment, screen draw commands, if-statements, labels, gotos).
- `Expression`: Arithmetic and comparison expressions.
- `Simple`, `Summand`, `Factor`: Helper non-terminals to parse expressions.