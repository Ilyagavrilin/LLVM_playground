#include "../task_1/sim.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

// Constants for register file and stack sizes
constexpr int REG_FILE_SIZE = 16;
constexpr int REG_FP_INDEX = REG_FILE_SIZE;       // Index for Frame Pointer (FP)
constexpr int REG_SP_INDEX = REG_FILE_SIZE + 1;   // Index for Stack Pointer (SP)
constexpr int TOTAL_REG_SIZE = REG_FILE_SIZE + 2; // Total registers including FP and SP
constexpr int STACK_SIZE = 1024;

// Global variables for register file and stack memory
uint32_t REG_FILE[TOTAL_REG_SIZE] = {0}; // Initialize all registers to zero
uint32_t STACK[STACK_SIZE] = {0};        // Initialize stack memory to zero

// Function to load instructions, handling comments and labels
std::vector<std::string> loadInstructions(
    const std::string &filename,
    std::unordered_map<std::string, int> &labelMap) {

    std::vector<std::string> instructions;
    std::ifstream input(filename);
    std::string line;
    int lineNumber = 0;

    if (!input.is_open()) {
        std::cerr << "[ERROR] Can't open file " << filename << "\n";
        exit(EXIT_FAILURE);
    }

    while (std::getline(input, line)) {
        ++lineNumber;
        std::string cleanedLine;
        std::istringstream iss(line);
        bool isLabelLine = false;
        std::string labelName;

        while (iss) {
            std::string token;
            iss >> token;
            if (token.empty()) continue;

            // Skip comments starting with ';'
            if (token[0] == ';') break;

            // Remove inline comments
            size_t commentPos = token.find(';');
            if (commentPos != std::string::npos) {
                token = token.substr(0, commentPos);
                if (token.empty()) break;
            }

            // Store labels in the labelMap and mark this line as a label line
            if (token.back() == ':') {
                labelName = token.substr(0, token.size() - 1);
                isLabelLine = true;
                continue;
            }

            // Add the token to the cleaned line
            if (!cleanedLine.empty()) cleanedLine += " ";
            cleanedLine += token;
        }

        if (isLabelLine) {
            // Map label to the index of the next instruction
            labelMap[labelName] = instructions.size();
        }

        if (!cleanedLine.empty()) {
            instructions.push_back(cleanedLine);
        }
    }

    input.close();
    return instructions;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        outs() << "[ERROR] Need 1 argument: file with assembler code\n";
        return EXIT_FAILURE;
    }

    // Initialize LLVM components
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMInitializeNativeDisassembler();

    LLVMContext context;
    IRBuilder<> builder(context);
    auto module = std::make_unique<Module>("top", context);

    // Declare `regFile` as a zero-initialized global variable
    ArrayType *regFileType = ArrayType::get(builder.getInt32Ty(), TOTAL_REG_SIZE);
    GlobalVariable *regFile = new GlobalVariable(
        *module, regFileType, false, GlobalValue::ExternalLinkage,
        ConstantAggregateZero::get(regFileType), "regFile");

    // Declare `STACK` as a zero-initialized global variable
    ArrayType *stackType = ArrayType::get(builder.getInt32Ty(), STACK_SIZE);
    GlobalVariable *stack = new GlobalVariable(
        *module, stackType, false, GlobalValue::ExternalLinkage,
        ConstantAggregateZero::get(stackType), "stack");

    // Create the main function
    FunctionType *mainFuncType = FunctionType::get(builder.getVoidTy(), false);
    Function *mainFunc = Function::Create(
        mainFuncType, Function::ExternalLinkage, "main", module.get());
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entryBB);

    // Initialize SP and FP
    // Store initial SP value (STACK_SIZE) into regFile[REG_SP_INDEX]
    Value *spInitPtr = builder.CreateInBoundsGEP(
        regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_SP_INDEX)});
    builder.CreateStore(builder.getInt32(STACK_SIZE), spInitPtr);

    // Initialize FP to 0
    Value *fpInitPtr = builder.CreateInBoundsGEP(
        regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_FP_INDEX)});
    builder.CreateStore(builder.getInt32(0), fpInitPtr);

    // Load instructions from file with comments and labels
    std::unordered_map<std::string, int> labelMap;
    std::vector<std::string> instructions = loadInstructions(argv[1], labelMap);

    // Create a basic block for each instruction
    std::vector<BasicBlock *> instructionBBs;
    for (size_t i = 0; i < instructions.size(); ++i) {
        BasicBlock *bb = BasicBlock::Create(
            context, "inst_" + std::to_string(i), mainFunc);
        instructionBBs.push_back(bb);
    }

    // Map labels to their corresponding basic blocks
    std::unordered_map<std::string, BasicBlock *> labelBBMap;
    for (const auto &labelPair : labelMap) {
        int idx = labelPair.second;
        if (idx < static_cast<int>(instructions.size())) {
            labelBBMap[labelPair.first] = instructionBBs[idx];
        } else {
            // Label at the end of the code, create an exit block
            BasicBlock *exitBB = BasicBlock::Create(context, "exit", mainFunc);
            builder.SetInsertPoint(exitBB);
            builder.CreateRetVoid();
            labelBBMap[labelPair.first] = exitBB;
        }
    }

    // Start from entry block
    if (!instructions.empty()) {
        builder.CreateBr(instructionBBs[0]);
    } else {
        builder.CreateRetVoid();
    }

    // Process each instruction
    for (size_t pc = 0; pc < instructions.size(); ++pc) {
        builder.SetInsertPoint(instructionBBs[pc]);
        std::string instr = instructions[pc];
        std::istringstream iss(instr);
        std::string opcode;
        iss >> opcode;

        bool terminatorAdded = false; // Flag to check if terminator was added

        // Skip empty instructions
        if (opcode.empty()) {
            if (!instructionBBs[pc]->getTerminator()) {
                if (pc + 1 < instructions.size()) {
                    builder.CreateBr(instructionBBs[pc + 1]);
                } else {
                    builder.CreateRetVoid();
                }
            }
            continue;
        }

        // Handle different opcodes
        if (opcode == "MOV") {
            // Handle MOV instruction
            std::string dst, src;
            iss >> dst >> src;

            int dstRegIndex = (dst == "FP") ? REG_FP_INDEX :
                              (dst == "SP") ? REG_SP_INDEX :
                              std::stoi(dst.substr(1));
            Value *dstPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(dstRegIndex)});

            if (src == "FP" || src == "SP" || src[0] == 'R') {
                int srcRegIndex = (src == "FP") ? REG_FP_INDEX :
                                  (src == "SP") ? REG_SP_INDEX :
                                  std::stoi(src.substr(1));
                Value *srcPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(srcRegIndex)});
                Value *srcVal = builder.CreateLoad(builder.getInt32Ty(), srcPtr);
                builder.CreateStore(srcVal, dstPtr);
            } else {
                int immediate = std::stoi(src);
                Value *immVal = builder.getInt32(immediate);
                builder.CreateStore(immVal, dstPtr);
            }
        } else if (opcode == "ADD") {
            // Handle ADD instruction
            std::string dst, src1, src2;
            iss >> dst >> src1 >> src2;

            int dstRegIndex = std::stoi(dst.substr(1));
            Value *dstPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(dstRegIndex)});

            int src1RegIndex = std::stoi(src1.substr(1));
            Value *src1Ptr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(src1RegIndex)});
            Value *src1Val = builder.CreateLoad(builder.getInt32Ty(), src1Ptr);

            Value *result;
            if (src2[0] == 'R') {
                int src2RegIndex = std::stoi(src2.substr(1));
                Value *src2Ptr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(src2RegIndex)});
                Value *src2Val = builder.CreateLoad(builder.getInt32Ty(), src2Ptr);
                result = builder.CreateAdd(src1Val, src2Val);
            } else {
                int immediate = std::stoi(src2);
                Value *immVal = builder.getInt32(immediate);
                result = builder.CreateAdd(src1Val, immVal);
            }

            builder.CreateStore(result, dstPtr);
        } else if (opcode == "SUB") {
            // Handle SUB instruction
            std::string dst, src1, src2;
            iss >> dst >> src1 >> src2;

            int dstRegIndex = std::stoi(dst.substr(1));
            Value *dstPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(dstRegIndex)});

            int src1RegIndex = std::stoi(src1.substr(1));
            Value *src1Ptr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(src1RegIndex)});
            Value *src1Val = builder.CreateLoad(builder.getInt32Ty(), src1Ptr);

            int src2RegIndex = std::stoi(src2.substr(1));
            Value *src2Ptr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(src2RegIndex)});
            Value *src2Val = builder.CreateLoad(builder.getInt32Ty(), src2Ptr);

            Value *result = builder.CreateSub(src1Val, src2Val);
            builder.CreateStore(result, dstPtr);
        } else if (opcode == "MOD") {
            // Handle MOD instruction
            std::string dst, src, modVal;
            iss >> dst >> src >> modVal;

            int dstRegIndex = std::stoi(dst.substr(1));
            Value *dstPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(dstRegIndex)});

            int srcRegIndex = std::stoi(src.substr(1));
            Value *srcPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(srcRegIndex)});
            Value *srcVal = builder.CreateLoad(builder.getInt32Ty(), srcPtr);

            int modValue = std::stoi(modVal);
            Value *modValLLVM = builder.getInt32(modValue);

            Value *result = builder.CreateSRem(srcVal, modValLLVM);
            builder.CreateStore(result, dstPtr);
        } else if (opcode == "CMP") {
            // Handle CMP instruction
            std::string resultReg, reg1, reg2;
            iss >> resultReg >> reg1 >> reg2;

            int resultRegIndex = std::stoi(resultReg.substr(1));
            Value *resultPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(resultRegIndex)});

            int reg1Index = std::stoi(reg1.substr(1));
            Value *reg1Ptr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(reg1Index)});
            Value *reg1Val = builder.CreateLoad(builder.getInt32Ty(), reg1Ptr);

            int reg2Index = std::stoi(reg2.substr(1));
            Value *reg2Ptr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(reg2Index)});
            Value *reg2Val = builder.CreateLoad(builder.getInt32Ty(), reg2Ptr);

            Value *cmpResult = builder.CreateICmpSLT(reg1Val, reg2Val);
            Value *cmpInt = builder.CreateSelect(cmpResult, builder.getInt32(1), builder.getInt32(0));
            builder.CreateStore(cmpInt, resultPtr);
        } else if (opcode == "PUSH") {
            // Handle PUSH instruction
            std::string reg;
            iss >> reg;

            int regIndex = (reg == "FP") ? REG_FP_INDEX :
                           (reg == "SP") ? REG_SP_INDEX :
                           std::stoi(reg.substr(1));

            // Decrement SP
            Value *spPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_SP_INDEX)});
            Value *spVal = builder.CreateLoad(builder.getInt32Ty(), spPtr);
            Value *newSpVal = builder.CreateSub(spVal, builder.getInt32(1));
            builder.CreateStore(newSpVal, spPtr);

            // Check for stack overflow
            Value *overflowCond = builder.CreateICmpSLT(newSpVal, builder.getInt32(0));
            BasicBlock *overflowBB = BasicBlock::Create(context, "overflow", mainFunc);
            BasicBlock *noOverflowBB = BasicBlock::Create(context, "no_overflow", mainFunc);
            builder.CreateCondBr(overflowCond, overflowBB, noOverflowBB);

            // Overflow block
            builder.SetInsertPoint(overflowBB);
            FunctionType *abortFuncType = FunctionType::get(builder.getVoidTy(), false);
            FunctionCallee abortFunc = module->getOrInsertFunction("abort", abortFuncType);
            builder.CreateCall(abortFunc);
            builder.CreateUnreachable();

            // No overflow block
            builder.SetInsertPoint(noOverflowBB);

            // Store register value to stack
            Value *stackPtr = builder.CreateInBoundsGEP(
                stackType, stack, {builder.getInt32(0), newSpVal});

            Value *regPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(regIndex)});
            Value *regVal = builder.CreateLoad(builder.getInt32Ty(), regPtr);

            builder.CreateStore(regVal, stackPtr);

            // Branch to the next instruction
            BasicBlock *nextBB = nullptr;
            if (pc + 1 < instructions.size()) {
                nextBB = instructionBBs[pc + 1];
            } else {
                nextBB = BasicBlock::Create(context, "exit", mainFunc);
                builder.SetInsertPoint(nextBB);
                builder.CreateRetVoid();
            }
            builder.CreateBr(nextBB);
            terminatorAdded = true;

        } else if (opcode == "POP") {
            // Handle POP instruction
            std::string reg;
            iss >> reg;

            int regIndex = (reg == "FP") ? REG_FP_INDEX :
                           (reg == "SP") ? REG_SP_INDEX :
                           std::stoi(reg.substr(1));

            // Load SP
            Value *spPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_SP_INDEX)});
            Value *spVal = builder.CreateLoad(builder.getInt32Ty(), spPtr);

            // Check for stack underflow
            Value *underflowCond = builder.CreateICmpUGE(spVal, builder.getInt32(STACK_SIZE));
            BasicBlock *underflowBB = BasicBlock::Create(context, "underflow", mainFunc);
            BasicBlock *noUnderflowBB = BasicBlock::Create(context, "no_underflow", mainFunc);
            builder.CreateCondBr(underflowCond, underflowBB, noUnderflowBB);

            // Underflow block
            builder.SetInsertPoint(underflowBB);
            FunctionType *abortFuncType = FunctionType::get(builder.getVoidTy(), false);
            FunctionCallee abortFunc = module->getOrInsertFunction("abort", abortFuncType);
            builder.CreateCall(abortFunc);
            builder.CreateUnreachable();

            // No underflow block
            builder.SetInsertPoint(noUnderflowBB);

            // Load value from stack
            Value *stackPtr = builder.CreateInBoundsGEP(
                stackType, stack, {builder.getInt32(0), spVal});
            Value *stackVal = builder.CreateLoad(builder.getInt32Ty(), stackPtr);

            // Store value into register
            Value *regPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(regIndex)});
            builder.CreateStore(stackVal, regPtr);

            // Increment SP
            Value *newSpVal = builder.CreateAdd(spVal, builder.getInt32(1));
            builder.CreateStore(newSpVal, spPtr);

            // Branch to the next instruction
            BasicBlock *nextBB = nullptr;
            if (pc + 1 < instructions.size()) {
                nextBB = instructionBBs[pc + 1];
            } else {
                nextBB = BasicBlock::Create(context, "exit", mainFunc);
                builder.SetInsertPoint(nextBB);
                builder.CreateRetVoid();
            }
            builder.CreateBr(nextBB);
            terminatorAdded = true;

        } else if (opcode == "BR") {
            // Handle unconditional branch
            std::string label;
            iss >> label;

            if (labelBBMap.find(label) != labelBBMap.end()) {
                builder.CreateBr(labelBBMap[label]);
                terminatorAdded = true;
            } else {
                std::cerr << "[ERROR] Undefined label: " << label << "\n";
                exit(EXIT_FAILURE);
            }
        } else if (opcode == "BR_IF") {
            // Handle conditional branch
            std::string condReg, label;
            iss >> condReg >> label;

            int condRegIndex = std::stoi(condReg.substr(1));

            // Load condition register value
            Value *condPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(condRegIndex)});
            Value *condVal = builder.CreateLoad(builder.getInt32Ty(), condPtr);

            // Create condition (non-zero)
            Value *condition = builder.CreateICmpNE(condVal, builder.getInt32(0));

            BasicBlock *trueBB = labelBBMap[label];
            BasicBlock *falseBB = (pc + 1 < instructions.size()) ? instructionBBs[pc + 1] : nullptr;

            if (!trueBB) {
                std::cerr << "[ERROR] Undefined label: " << label << "\n";
                exit(EXIT_FAILURE);
            }

            if (!falseBB) {
                falseBB = BasicBlock::Create(context, "exit", mainFunc);
                builder.SetInsertPoint(falseBB);
                builder.CreateRetVoid();
            }

            builder.CreateCondBr(condition, trueBB, falseBB);
            terminatorAdded = true;
        } else if (opcode == "CALL") {
            // Handle CALL instruction
            std::string function;
            iss >> function;

            if (function == "SIM_PUT_PIXEL") {
                // Handle SIM_PUT_PIXEL call
                std::string xReg, yReg, colorReg;
                iss >> xReg >> yReg >> colorReg;

                int xIndex = std::stoi(xReg.substr(1));
                int yIndex = std::stoi(yReg.substr(1));
                int colorIndex = std::stoi(colorReg.substr(1));

                // Declare simPutPixel function
                FunctionType *simPutPixelType = FunctionType::get(
                    builder.getVoidTy(),
                    {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee simPutPixelFunc = module->getOrInsertFunction("simPutPixel", simPutPixelType);

                // Load arguments
                Value *xPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(xIndex)});
                Value *xVal = builder.CreateLoad(builder.getInt32Ty(), xPtr);

                Value *yPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(yIndex)});
                Value *yVal = builder.CreateLoad(builder.getInt32Ty(), yPtr);

                Value *colorPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(colorIndex)});
                Value *colorVal = builder.CreateLoad(builder.getInt32Ty(), colorPtr);

                // Call simPutPixel
                builder.CreateCall(simPutPixelFunc, {xVal, yVal, colorVal});
            } else if (function == "SIM_RAND") {
                // Handle SIM_RAND call
                std::string reg;
                iss >> reg;

                int regIndex = std::stoi(reg.substr(1));

                // Declare simRand function
                FunctionType *simRandType = FunctionType::get(builder.getInt32Ty(), false);
                FunctionCallee simRandFunc = module->getOrInsertFunction("simRand", simRandType);

                // Call simRand
                Value *randVal = builder.CreateCall(simRandFunc);

                // Store result in register
                Value *regPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(regIndex)});
                builder.CreateStore(randVal, regPtr);
            } else if (function == "SIM_FLUSH") {
                // Handle SIM_FLUSH call
                FunctionType *simFlushType = FunctionType::get(builder.getVoidTy(), false);
                FunctionCallee simFlushFunc = module->getOrInsertFunction("simFlush", simFlushType);
                builder.CreateCall(simFlushFunc);
            } else {
                // Handle user-defined function calls
                if (labelBBMap.find(function) == labelBBMap.end()) {
                    std::cerr << "[ERROR] Undefined function: " << function << "\n";
                    exit(EXIT_FAILURE);
                }

                // Push return address (pc + 1) onto the stack

                // Decrement SP
                Value *spPtr = builder.CreateInBoundsGEP(
                    regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_SP_INDEX)});
                Value *spVal = builder.CreateLoad(builder.getInt32Ty(), spPtr);
                Value *newSpVal = builder.CreateSub(spVal, builder.getInt32(1));
                builder.CreateStore(newSpVal, spPtr);

                // Check for stack overflow
                Value *overflowCond = builder.CreateICmpSLT(newSpVal, builder.getInt32(0));
                BasicBlock *overflowBB = BasicBlock::Create(context, "overflow_call", mainFunc);
                BasicBlock *noOverflowBB = BasicBlock::Create(context, "no_overflow_call", mainFunc);
                builder.CreateCondBr(overflowCond, overflowBB, noOverflowBB);

                // Overflow block
                builder.SetInsertPoint(overflowBB);
                FunctionType *abortFuncType = FunctionType::get(builder.getVoidTy(), false);
                FunctionCallee abortFunc = module->getOrInsertFunction("abort", abortFuncType);
                builder.CreateCall(abortFunc);
                builder.CreateUnreachable();

                // No overflow block
                builder.SetInsertPoint(noOverflowBB);

                // Store return address (pc + 1)
                int returnAddress = pc + 1;
                Value *stackPtr = builder.CreateInBoundsGEP(
                    stackType, stack, {builder.getInt32(0), newSpVal});
                builder.CreateStore(builder.getInt32(returnAddress), stackPtr);

                // Branch to function label
                builder.CreateBr(labelBBMap[function]);
                terminatorAdded = true;
            }
        } else if (opcode == "RET") {
            // Handle RET instruction

            // Load SP
            Value *spPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(REG_SP_INDEX)});
            Value *spVal = builder.CreateLoad(builder.getInt32Ty(), spPtr);

            // Check for stack underflow
            Value *underflowCond = builder.CreateICmpUGE(spVal, builder.getInt32(STACK_SIZE));
            BasicBlock *underflowBB = BasicBlock::Create(context, "underflow_ret", mainFunc);
            BasicBlock *noUnderflowBB = BasicBlock::Create(context, "no_underflow_ret", mainFunc);
            builder.CreateCondBr(underflowCond, underflowBB, noUnderflowBB);

            // Underflow block
            builder.SetInsertPoint(underflowBB);
            FunctionType *abortFuncType = FunctionType::get(builder.getVoidTy(), false);
            FunctionCallee abortFunc = module->getOrInsertFunction("abort", abortFuncType);
            builder.CreateCall(abortFunc);
            builder.CreateUnreachable();

            // No underflow block
            builder.SetInsertPoint(noUnderflowBB);

            // Load return address from stack
            Value *stackPtr = builder.CreateInBoundsGEP(
                stackType, stack, {builder.getInt32(0), spVal});
            Value *returnAddrVal = builder.CreateLoad(builder.getInt32Ty(), stackPtr);

            // Increment SP
            Value *newSpVal = builder.CreateAdd(spVal, builder.getInt32(1));
            builder.CreateStore(newSpVal, spPtr);

            // Create switch for indirect branching
            BasicBlock *defaultBB = BasicBlock::Create(context, "default_ret", mainFunc);
            builder.SetInsertPoint(defaultBB);
            builder.CreateRetVoid();

            builder.SetInsertPoint(noUnderflowBB);
            SwitchInst *switchInst = builder.CreateSwitch(returnAddrVal, defaultBB, instructionBBs.size());

            for (size_t i = 0; i < instructionBBs.size(); ++i) {
                switchInst->addCase(builder.getInt32(i), instructionBBs[i]);
            }
            terminatorAdded = true;
        } else if (opcode == "EXIT") {
            // Handle EXIT instruction
            builder.CreateRetVoid();
            terminatorAdded = true;
        } else {
            std::cerr << "[ERROR] Unknown opcode: " << opcode << "\n";
            exit(EXIT_FAILURE);
        }

        // At the end, if no terminator was added, add one
        if (!terminatorAdded && !instructionBBs[pc]->getTerminator()) {
            if (pc + 1 < instructions.size()) {
                builder.CreateBr(instructionBBs[pc + 1]);
            } else {
                builder.CreateRetVoid();
            }
        }
    }

    // Verify the module
    if (verifyModule(*module, &errs())) {
        errs() << "[ERROR] Module verification failed\n";
        return EXIT_FAILURE;
    }

    // Create the ExecutionEngine
    std::string errorStr;
    ExecutionEngine *ee = EngineBuilder(std::move(module))
                              .setErrorStr(&errorStr)
                              .setEngineKind(EngineKind::JIT)
                              .create();

    if (!ee) {
        std::cerr << "[ERROR] Failed to create ExecutionEngine: " << errorStr << "\n";
        return EXIT_FAILURE;
    }

    // Map external functions
    ee->InstallLazyFunctionCreator([=](const std::string &fnName) -> void * {
        if (fnName == "simPutPixel") return reinterpret_cast<void *>(&simPutPixel);
        if (fnName == "simRand") return reinterpret_cast<void *>(&simRand);
        if (fnName == "simFlush") return reinterpret_cast<void *>(&simFlush);
        if (fnName == "abort") return reinterpret_cast<void *>(&abort);
        return nullptr;
    });

    ee->finalizeObject();

    // Initialize simulation
    simInit();

    // Run the main function
    ee->runFunctionAsMain(mainFunc, {}, nullptr);

    // Exit simulation
    simExit();
    return EXIT_SUCCESS;
}
