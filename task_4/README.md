# Task 4
In this task we created our own Assembler, and make LLVM handlers for it.
## Instructions definitions
| Opcode Name | Opcode with Arguments     | Result Evaluation (Formula)                                  |
|-------------|---------------------------|--------------------------------------------------------------|
| MOV         | MOV dst src               | dst = src                                                    |
|             | MOV dst imm               | dst = imm                                                    |
| ADD         | ADD dst src1 src2         | dst = src1 + src2                                            |
|             | ADD dst src1 imm          | dst = src1 + imm                                             |
| SUB         | SUB dst src1 src2         | dst = src1 - src2                                            |
| MOD         | MOD dst src mod_value     | dst = src % mod_value                                        |
| CMP         | CMP result_reg reg1 reg2  | result_reg = (reg1 &gt;= reg2) ? 0 : 1                       |
| PUSH        | PUSH reg                  | SP = SP - 1; STACK[SP] = REG_FILE[reg]                       |
| POP         | POP reg                   | REG_FILE[reg] = STACK[SP]; SP = SP + 1                       |
| CALL        | CALL function_name [args] | SP = SP - 1; STACK[SP] = return_address; goto function_name  |
| RET         | RET                       | return_address = STACK[SP]; SP = SP + 1; goto return_address |
| BR_IF       | BR_IF condition_reg label | if (condition_reg ≠ 0) goto label                            |
| BR          | BR label                  | goto label                                                   |

## Build and Run
```bash
$> cmake -B build/
$> ./build/ASM_SIM app.s 
```
### Problems
Now I have some problems with branching, it is hard to implement it.