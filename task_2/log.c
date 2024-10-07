#include <stdio.h>

void instructionLogger(char* InstructionName, long int instructionID) {
    printf("[INSTR] #%ld: %s\n", instructionID, InstructionName);
}

void usesLogger(char* lhs, char* rhs) {
    printf("[USE] %s <- %s\n", lhs, rhs);
}